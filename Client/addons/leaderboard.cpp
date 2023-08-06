#include "client.h"
#include "leaderboard.h"

#include <codecvt>
#include <algorithm>

#include "../engine.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../menu.h"
#include "../util.h"
#include "../settings.h"

// clang-format off

static auto enabled = false;
static std::string levelName;

static char nameInput[0x20] = {0};
static std::string playerName;

static Leaderboard::TimeTrial oldTimeTrial;
static Leaderboard::Speedrun oldSpeedrun;
static Leaderboard::Pawn oldPawn;
static Leaderboard::Controller oldController;

// Timetrials
static std::vector<Checkpoint> checkpoints;
static int checkpointRespawnCount = 0;
static float checkpointTopSpeed = 0.0f;
static float checkpointPreviousDistance = 0.0f;
static float checkpointPreviousTime = 0.0f;
static float checkpointPreviousRealTime = 0.0f;

// Timestamps in unix time. These are used to know if the run was started properly or not
static std::chrono::milliseconds startTimestamp = std::chrono::milliseconds(0);
static std::chrono::milliseconds endTimestamp = std::chrono::milliseconds(0);

// Chapter Speedruns
static int reactionTimeUses = 0;
static int playerDeaths = 0;

// Used by both timetrial and chapter speedrun
static float overallTopSpeed = 0.0f;

static void Save(Classes::ATdPlayerPawn* pawn, Classes::ATdPlayerController* controller, Classes::ATdSPTimeTrialGame* timetrial, Classes::ATdSPLevelRace* speedrun) {
    if (pawn) {
        oldPawn.Health = pawn->Health;
        oldPawn.bSRPauseTimer = pawn->bSRPauseTimer;
    }

    if (controller) {
        oldController.bReactionTime = controller->bReactionTime;
        oldController.ReactionTimeEnergy = controller->ReactionTimeEnergy;
    }

    if (speedrun) {
        oldSpeedrun.bRaceOver = speedrun->bRaceOver;
        oldSpeedrun.TimeAttackClock = speedrun->TdGameData->TimeAttackClock;
        oldSpeedrun.TimeAttackDistance = speedrun->TdGameData->TimeAttackDistance;
    }

    if (timetrial) {
        oldTimeTrial.RaceFinishLineTime = timetrial->RaceFinishLineTime;             
        oldTimeTrial.RaceFinishLineTimer = timetrial->RaceFinishLineTimer;                
        oldTimeTrial.RaceCountDownTime = timetrial->RaceCountDownTime;      
        oldTimeTrial.RaceCountDownTimer = timetrial->RaceCountDownTimer;    
        oldTimeTrial.NumCheckPoints = timetrial->NumCheckPoints;
        oldTimeTrial.NumPassedCheckPoints = timetrial->NumPassedCheckPoints;
        oldTimeTrial.NumPassedTimerCheckPoints = timetrial->NumPassedTimerCheckPoints;  
        oldTimeTrial.LastCheckpointTimeStamp = timetrial->LastCheckpointTimeStamp;  
        oldTimeTrial.LastPlayerResetTime = timetrial->LastPlayerResetTime; 
        oldTimeTrial.CurrentTackData = timetrial->CurrentTackData;  
        oldTimeTrial.CurrentTimeData = timetrial->CurrentTimeData;  
        oldTimeTrial.TimeDataToBeat = timetrial->TimeDataToBeat;
        oldTimeTrial.RaceStartTimeStamp = timetrial->RaceStartTimeStamp;  
        oldTimeTrial.RaceEndTimeStamp = timetrial->RaceEndTimeStamp;
        oldTimeTrial.PlayerDistance = timetrial->PlayerDistance;
    }
}

static void ResetValues() {
    checkpoints.clear();
    checkpoints.shrink_to_fit();

    checkpointRespawnCount = 0;
    checkpointTopSpeed = 0.0f;
    checkpointPreviousTime = 0.0f;
    checkpointPreviousDistance = 0.0f;
    checkpointPreviousRealTime = 0.0f;

    reactionTimeUses = 0;
    playerDeaths = 0;

    overallTopSpeed = 0.0f;

    startTimestamp = std::chrono::milliseconds(0);
    endTimestamp = std::chrono::milliseconds(0);
}

static void LeaderboardTab() { 
    if (ImGui::Checkbox("Enabled", &enabled)) {
        Settings::SetSetting("race", "enabled", enabled);
    }

    if (!enabled) {
        return;
    }

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 6.0f));
    ImGui::Text("Name");
    ImGui::SameLine();

    if (ImGui::InputText("##leaderboard-name-input", nameInput, sizeof(nameInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (playerName != nameInput) {
            bool empty = true;
            for (auto c : std::string(nameInput)) {
                if (!isblank(c)) {
                    empty = false;
                    break;
                }
            }

            if (!empty) {
                playerName = nameInput;
                Settings::SetSetting("race", "playerName", playerName);
            }
        }
    }
}

static void SendJsonData(json jsonData) {
    jsonData.push_back({"Username", playerName});
    jsonData.push_back({"StartTimestamp", startTimestamp.count()});
    jsonData.push_back({"EndTimestamp", endTimestamp.count()});

    Client client;
    client.SendJsonMessage({
        {"type", "post"},
        {"body", jsonData.dump()},
    });
}

static std::chrono::milliseconds GetTimeInMillisecondsSinceEpoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

static void OnTickTimeTrial(Classes::ATdSPTimeTrialGame* timetrial, float deltaTime) {
    if (timetrial->RaceCountDownTimer == timetrial->RaceCountDownTime + 1 && timetrial->RaceCountDownTimer != oldTimeTrial.RaceCountDownTimer) {
        printf("timetrial: countdown started\n");

        // TODO: Find a way to reset the world's time seconds without side effects
        // * The characters mesh gets messed up if you use world->TimeSeconds = 0.0f

        ResetValues();
    }

    if (timetrial->RaceCountDownTimer == 0 && timetrial->RaceCountDownTimer != oldTimeTrial.RaceCountDownTimer && timetrial->RaceStartTimeStamp != oldTimeTrial.RaceStartTimeStamp && timetrial->RaceStartTimeStamp != -1) {
        printf("timetrial: timetrial started\n");

        startTimestamp = GetTimeInMillisecondsSinceEpoch();
    }

    if (timetrial->CurrentTimeData.TotalTime == 0.0f && timetrial->RacingPawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) {
        const auto speed = sqrtf(powf(timetrial->RacingPawn->Velocity.X, 2) + powf(timetrial->RacingPawn->Velocity.Y, 2)) * 0.036f;

        // TODO: Save extra data such as the time, distance, and etc
        if (speed > checkpointTopSpeed) {
            checkpointTopSpeed = speed;
        }
            
        if (speed > overallTopSpeed) {
            overallTopSpeed = speed;
        }
    }

    if (timetrial->RaceCountDownTimer == 0 && timetrial->CurrentTimeData.TotalTime == 0.0f && timetrial->LastPlayerResetTime != oldTimeTrial.LastPlayerResetTime) {
        checkpointRespawnCount++;
    }

    if (timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) {
        const float distance = (timetrial->PlayerDistance / (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints ? 1 : 100)) - checkpointPreviousDistance;
        const float time = timetrial->WorldInfo->TimeSeconds - timetrial->RaceStartTimeStamp - checkpointPreviousTime;
        const float avgspeed = (distance / time) * 3.6f;
        
        Checkpoint chp;
        chp.avgspeed = avgspeed;
        chp.topspeed = checkpointTopSpeed;
        chp.timedCheckpoint = timetrial->NumPassedTimerCheckPoints > oldTimeTrial.NumPassedTimerCheckPoints;
        chp.intermediateDistance = distance;
        chp.accumulatedIntermediateDistance = distance + checkpointPreviousDistance;
        chp.intermediateTime = time;
        chp.accumulatedIntermediateTime = time + checkpointPreviousTime;
        chp.respawnCount = checkpointRespawnCount;

        checkpoints.push_back(chp);

        checkpointRespawnCount = 0;
        checkpointTopSpeed = 0.0f;
        checkpointPreviousTime = time + checkpointPreviousTime;
        checkpointPreviousDistance = distance + checkpointPreviousDistance;
    }

    if (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints && timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) {
        printf("timetrial: timetrial finished\n");
        endTimestamp = GetTimeInMillisecondsSinceEpoch();

        if (checkpoints.size() != timetrial->NumPassedCheckPoints || startTimestamp == std::chrono::milliseconds(0)) {
            printf("timetrial: invalid data!\n");
            return;
        }

        json jsonIntermediateDistances;
        json jsonCheckpoints;
        json jsonCheckpointData;

        for (int i = 0; i < timetrial->NumPassedTimerCheckPoints; i++) {
			jsonIntermediateDistances.push_back(timetrial->CurrentTackData.IntermediateDistance[i] / 100.0f);
        }

        for (int i = 0; i < timetrial->NumCheckPoints; i++) {
            jsonCheckpoints.push_back({
                {"AvgSpeed", checkpoints[i].avgspeed},
                {"TopSpeed", checkpoints[i].topspeed},
                {"TimedCheckpoint", checkpoints[i].timedCheckpoint == true ? "true" : "false"},
                {"IntermediateDistance", checkpoints[i].intermediateDistance},
                {"AccumulatedIntermediateDistance", checkpoints[i].accumulatedIntermediateDistance},
                {"IntermediateTime", checkpoints[i].intermediateTime},
                {"AccumulatedIntermediateTime", checkpoints[i].accumulatedIntermediateTime},
                {"RespawnCount", checkpoints[i].respawnCount}
            });
        }

        for (int i = 0; i < timetrial->NumPassedCheckPoints; i++) {
            const auto track = static_cast<Classes::ATdTimerCheckpoint*>(timetrial->CheckpointManager->ActiveTrack[i]);

            json jsonBelongToTracks;

            for (size_t j = 0; j < track->BelongToTracks.Num(); j++) {
			    jsonBelongToTracks.push_back({
                    {"TrackIndex", track->BelongToTracks[j].TrackIndex.GetValue()},
                    {"OrderIndex", track->BelongToTracks[j].OrderIndex},
                    {"bNoIntermediateTime", track->BelongToTracks[j].bNoIntermediateTime == true ? "true" : "false"},
                    {"bLastCheckpoint", track->BelongToTracks[j].bLastCheckpoint == true ? "true" : "false"}
                });
		    }

            jsonCheckpointData.push_back({
                // UObject
                {"FullName", track->GetFullName()},

                // AActor
                {"X", track->Location.X},
                {"Y", track->Location.Y},
                {"Z", track->Location.Z},
                {"Pitch", track->Rotation.Pitch},
                {"Yaw", track->Rotation.Yaw},
                {"Roll", track->Rotation.Roll},

                // ATdTimerCheckpoint
                {"BelongToTracks", jsonBelongToTracks},
                {"CustomHeight", track->CustomHeight},
                {"CustomWidthScale", track->CustomWidthScale},
                {"bNoRespawn", track->bNoRespawn == true ? "true" : "false"},
                {"InitialHeight", track->InitialHeight},
                {"InitialRadius", track->InitialRadius},

                // ATdPlaceableCheckpoint
                {"bShouldBeBased", track->bShouldBeBased == true ? "true" : "false"},
            });
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", levelName},

            // Timetrial data
            {"ActiveTTStretch", timetrial->ActiveTTStretch.GetValue()},
            {"QualifyingTime", timetrial->QualifyingTime},
            {"StarRatingTimes", {
                {"3", timetrial->StarRatingTimes[0]},
                {"2", timetrial->StarRatingTimes[1]},
                {"1", timetrial->StarRatingTimes[2]}}
            },
            {"TotalTime", timetrial->CurrentTimeData.TotalTime},
            {"PlayerDistance", timetrial->PlayerDistance},
            {"AvgSpeed", (timetrial->PlayerDistance / timetrial->CurrentTimeData.TotalTime) * 3.6f},

            // Custom data
            {"CheckpointData", jsonCheckpoints},
            {"TopSpeed", overallTopSpeed},

            // These are sent in order to verify that the run is valid
            {"TrackData", {
                {"TotalDistance", timetrial->CurrentTackData.TotalDistance / 100.0f},
                {"IntermediateDistances", jsonIntermediateDistances},
                {"CheckpointData", jsonCheckpointData}}
            }
        };

        SendJsonData(jsonData);
        ResetValues();
    }
}

static void OnTickSpeedRun(Classes::ATdSPLevelRace* speedrun, float deltaTime) {
    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();
    const bool isTimerAtZero = speedrun->TdGameData->TimeAttackClock == 0.0f && pawn->WorldInfo->RealTimeSeconds == 0.0f;
    
    if (isTimerAtZero && speedrun->TdGameData->TimeAttackDistance != 0.0f) {
        printf("speedrun: \"TimeAttackDistance\" is not zero! Setting it to 0.0f\n");
        speedrun->TdGameData->TimeAttackDistance = 0.0f;
    }

    if (isTimerAtZero && speedrun->TdGameData->TimeAttackDistance == 0.0f) {
        printf("speedrun: speedrun started\n");
        ResetValues();
        startTimestamp = GetTimeInMillisecondsSinceEpoch();
    }

    if (!speedrun->bRaceOver && pawn->bAllowMoveChange) {
        float speed = sqrtf(powf(pawn->Velocity.X, 2) + powf(pawn->Velocity.Y, 2)) * 0.036f;

        if (speed > overallTopSpeed && pawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) {
            overallTopSpeed = speed;
        }
    }

    if (controller->bReactionTime && controller->bReactionTime != oldController.bReactionTime && !speedrun->bRaceOver) {
        printf("speedrun: reactiontime used\n");
        reactionTimeUses++;
    }

    if (pawn->Health <= 0 && pawn->Health != oldPawn.Health) {
        printf("speedrun: player died\n");
        playerDeaths++;
    }

    if (speedrun->bRaceOver && speedrun->bRaceOver != oldSpeedrun.bRaceOver) {
        printf("speedrun: speedrun finished\n");
        endTimestamp = GetTimeInMillisecondsSinceEpoch();

        if (startTimestamp == std::chrono::milliseconds(0)) {
            printf("speedrun: invalid data!\n");
            return;
        }

        json jsonData = {
            {"MapName", levelName},
            {"TimeAttackClock", speedrun->TdGameData->TimeAttackClock},
            {"TimeAttackDistance", speedrun->TdGameData->TimeAttackDistance},
            {"TimeSeconds", pawn->WorldInfo->TimeSeconds},
            {"RealTimeSeconds", pawn->WorldInfo->RealTimeSeconds},
            {"AvgSpeed", (speedrun->TdGameData->TimeAttackDistance / speedrun->TdGameData->TimeAttackClock) * 3.6f},
            {"TopSpeed", overallTopSpeed},
            {"ReactionTimeUsed", reactionTimeUses},
            {"PlayerDeaths", playerDeaths}
        };

        SendJsonData(jsonData);
        ResetValues();
    }
}

static void OnTick(float deltaTime) {
    if (!enabled || playerName.empty()) {
        return;
    }

    if (levelName.empty() || levelName == Map_MainMenu) {
        return;
    }

    const auto world = Engine::GetWorld();
    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();

    if (!pawn || !world || !controller) {
        return;
    }

    if (world && world->Game) {
        std::string game = world->Game->GetName();

        if (game.find("TdSPTimeTrialGame") != -1) {
            auto timetrial = static_cast<Classes::ATdSPTimeTrialGame*>(world->Game);

            OnTickTimeTrial(timetrial, deltaTime);
            Save(nullptr, nullptr, timetrial, nullptr);
        } 
        else if (game.find("TdSPLevelRace") != -1) {
            auto speedrun = static_cast<Classes::ATdSPLevelRace*>(world->Game);

            OnTickSpeedRun(speedrun, deltaTime);
            Save(pawn, controller, nullptr, speedrun);
        }
    }
}

bool Leaderboard::Initialize() {
    enabled = Settings::GetSetting("race", "enabled", false);
    playerName = Settings::GetSetting("race", "playerName", "").get<std::string>();
    strncpy_s(nameInput, sizeof(nameInput) - 1, playerName.c_str(), sizeof(nameInput) - 1);

	Menu::AddTab("Leaderboard", LeaderboardTab);
    Engine::OnTick(OnTick);

	Engine::OnPreLevelLoad([](const wchar_t *levelNameW) {
		levelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(levelNameW);

		std::transform(levelName.begin(), levelName.end(), levelName.begin(), [](char c) { 
			return tolower(c); 
		});
	});

	return true;
}

std::string Leaderboard::GetName() { return "Leaderboard"; }