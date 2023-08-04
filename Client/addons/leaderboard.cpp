#include "client.h"
#include "leaderboard.h"

#include <codecvt>
#include <algorithm>
#include <chrono>

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

static Checkpoint checkpoints;
static float checkpointTopSpeed = 0.0f;
static float previousDistance = 0.0f;
static float previousTime = 0.0f;

static std::chrono::milliseconds startTimestamp = std::chrono::milliseconds(0);
static std::chrono::milliseconds endTimestamp = std::chrono::milliseconds(0);

static int reactionTimeUses = 0;
static int playerDeaths = 0;
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
    checkpoints.Clear();
    checkpointTopSpeed = 0.0f;
    previousTime = 0.0f;
    previousDistance = 0.0f;

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
        printf("timetrial: countdown starting\n");

        // TODO: Find a way to reset the world's time seconds without side effects
        // * The characters mesh gets messed up if you use world->TimeSeconds = 0.0f

        ResetValues();
    }

    if (timetrial->RaceStartTimeStamp != oldTimeTrial.RaceStartTimeStamp && timetrial->RaceStartTimeStamp != -1) {
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

    if (timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) {
        const float distance = (timetrial->PlayerDistance / (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints ? 1 : 100)) - previousDistance;
        const float time = (timetrial->WorldInfo->TimeSeconds - timetrial->RaceStartTimeStamp) - previousTime;
        const float avgspeed = (distance / time) * 3.6;
        
        checkpoints.avgspeed.push_back(avgspeed);
        checkpoints.topspeed.push_back(checkpointTopSpeed);
        checkpoints.timedCheckpoint.push_back(timetrial->NumPassedTimerCheckPoints > oldTimeTrial.NumPassedTimerCheckPoints);
        checkpoints.intermediateDistances.push_back(distance);
        checkpoints.accumulatedIntermediateDistances.push_back(distance + previousDistance);
        checkpoints.intermediateTimes.push_back(time);
        checkpoints.accumulatedIntermediateTimes.push_back(time + previousTime);

        checkpointTopSpeed = 0.0f;
        previousTime = time + previousTime;
        previousDistance = distance + previousDistance;
    }

    if (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints && timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) {
        printf("timetrial: timetrial finished\n");
        endTimestamp = GetTimeInMillisecondsSinceEpoch();

        if (!checkpoints.IsValid(timetrial->NumPassedCheckPoints) || startTimestamp == std::chrono::milliseconds(0)) {
            printf("timetrial: invalid data!\n");
            return;
        }

        json jsonTrackData;
        json jsonCheckpoints;

        for (int i = 0; i < timetrial->NumCheckPoints; i++) {
            json jsonData = {
                {"AvgSpeed", checkpoints.avgspeed[i]},
                {"TopSpeed", checkpoints.topspeed[i]},
                {"TimedCheckpoint", checkpoints.timedCheckpoint[i] == true ? "true" : "false"},
                {"IntermediateDistances", checkpoints.intermediateDistances[i]},
                {"AccumulatedIntermediateDistances", checkpoints.accumulatedIntermediateDistances[i]},
                {"IntermediateTimes", checkpoints.intermediateTimes[i]},
                {"AccumulatedIntermediateTimes", checkpoints.accumulatedIntermediateTimes[i]}
            };

            jsonCheckpoints.push_back(jsonData);
        }

        for (size_t i = 0; i < timetrial->CurrentTackData.IntermediateDistance.Num(); i++) {
			jsonTrackData.push_back(timetrial->CurrentTackData.IntermediateDistance[i] / 100.0f);
		}

        json jsonData = {
            {"MapName", levelName},
            {"ActiveTTStretch", timetrial->ActiveTTStretch.GetValue()},
            {"QualifyingTime", timetrial->QualifyingTime},
            {"StarRatingTimes", {
                {"3", timetrial->StarRatingTimes[0]},
                {"2", timetrial->StarRatingTimes[1]},
                {"1", timetrial->StarRatingTimes[2]}}
            },
            {"TrackData", {
                {"TotalDistance", timetrial->CurrentTackData.TotalDistance / 100.0f},
                {"IntermediateDistance", jsonTrackData}}
            },
            {"TotalTime", timetrial->CurrentTimeData.TotalTime},
            {"Distance", timetrial->PlayerDistance},
            {"AvgSpeed", (timetrial->PlayerDistance / timetrial->CurrentTimeData.TotalTime) * 3.6f},
            {"TopSpeed", overallTopSpeed},
            {"CheckpointData", jsonCheckpoints}
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