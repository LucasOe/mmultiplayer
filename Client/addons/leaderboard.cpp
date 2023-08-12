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

#define MS_TO_KPH   3.6f    // Meter per seconds to Kilometers per hour
#define CMS_TO_KPH  0.036f  // Centimeter per seconds to Kilometers per hour

// clang-format off

static bool enabled = false;
static bool submitRun = true;
static char nameInput[0x20] = {0};
static std::string playerName;
static std::string levelName;

static Leaderboard::Old::TimeTrial oldTimeTrial;
static Leaderboard::Old::Speedrun oldSpeedrun;
static Leaderboard::Old::Pawn oldPawn;
static Leaderboard::Old::Controller oldController;

// Time Trials
static std::vector<TimeTrialInfo> timeTrialCheckpointInfo;
static float timeTrialCheckpointPreviousTime = 0.0f;
static float timeTrialCheckpointPreviousDistance = 0.0f;
static float timeTrialRealTimeStarted = 0.0f;
static Info timeTrialCheckpointTopSpeedInfo = {0};
static std::vector<Info> timeTrialCheckpointRespawnInfo;

// Chapter Speedruns
static std::vector<Info> speedrunCheckpointInfo;
static std::vector<Info> speedrunReactionTimeInfo;
static std::vector<Info> speedrunPlayerDeathsInfo;

// Timestamps in unix time. These are used to know if the run was started properly or not
static std::chrono::milliseconds startTimeStampUnix = std::chrono::milliseconds(0);
static std::chrono::milliseconds endTimeStampUnix = std::chrono::milliseconds(0);
static float topSpeed = 0.0f;

static void Save(const Classes::ATdPlayerPawn* pawn, const Classes::ATdPlayerController* controller, const Classes::ATdSPTimeTrialGame* timetrial, const Classes::ATdSPLevelRace* speedrun) {
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
        oldSpeedrun.ActiveCheckpointWeight = speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight;
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
    timeTrialCheckpointInfo.clear();
    timeTrialCheckpointPreviousTime = 0.0f;
    timeTrialCheckpointPreviousDistance = 0.0f;
    timeTrialRealTimeStarted = 0.0f;
    timeTrialCheckpointTopSpeedInfo = {0};
    timeTrialCheckpointRespawnInfo.clear();

    speedrunCheckpointInfo.clear();
    speedrunReactionTimeInfo.clear();
    speedrunPlayerDeathsInfo.clear();

    startTimeStampUnix = std::chrono::milliseconds(0);
    endTimeStampUnix = std::chrono::milliseconds(0);
    topSpeed = 0.0f;
}

static void LeaderboardTab() { 
    if (ImGui::Checkbox("Enabled", &enabled)) {
        Settings::SetSetting("race", "enabled", enabled);
    }

    if (!enabled) {
        return;
    }

    if (ImGui::Checkbox("Submit Run", &submitRun)) {
        Settings::SetSetting("race", "submitRun", submitRun);
    }

    ImGui::SeperatorWithPadding(2.5f);
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

            Settings::SetSetting("race", "playerName", playerName = (!empty ? nameInput : ""));
        }
    }

    ImGui::SeperatorWithPadding(2.5f);

    if (!playerName.empty() && submitRun) {
        ImGui::Text("Your runs that you finish will be uploaded to a server");
    } else {
        ImGui::Text("Your runs that you finish will >> NOT << be uploaded to a server");
        if (playerName.empty()){
            ImGui::Text("- The name can't be empty!");
        } else {
            ImGui::Text("- \"Submit Run\" is off");
        }
    }
}

static void SendJsonData(json jsonData) {
    jsonData.push_back({"Username", playerName});
    jsonData.push_back({"StartTimeStampUnix", startTimeStampUnix.count()});
    jsonData.push_back({"EndTimeStampUnix", endTimeStampUnix.count()});

    if (submitRun) {
        printf("data sent\n\n");

        Client client;
        client.SendJsonMessage({
            {"type", "post"},
            {"body", jsonData.dump()},
        });
    }

    ResetValues();
}

static std::chrono::milliseconds GetTimeInMillisecondsSinceEpoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

static void OnTickTimeTrial(Classes::ATdSPTimeTrialGame* timetrial, const float deltaTime) {
    const float distance = (timetrial->PlayerDistance / (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints ? 1 : 100)) - timeTrialCheckpointPreviousDistance;
    const float time = timetrial->WorldInfo->TimeSeconds - (timetrial->RaceStartTimeStamp != -1 ? timetrial->RaceStartTimeStamp : 0.0f) - timeTrialCheckpointPreviousTime;
    const float speed = timetrial->RacingPawn ? sqrtf(powf(timetrial->RacingPawn->Velocity.X, 2) + powf(timetrial->RacingPawn->Velocity.Y, 2)) * CMS_TO_KPH : 0.0f;

    // Info about the current tick such as time, avgspeed, and etc
    Info info;
    info.time = time + timeTrialCheckpointPreviousTime;
    info.distance = distance + timeTrialCheckpointPreviousDistance;
    info.avgspeed = (info.distance / info.time) * MS_TO_KPH;
    info.topspeed = timeTrialCheckpointTopSpeedInfo.topspeed;
    info.intermediateTime = time;
    info.intermediateDistance = distance;
    info.unixTime = GetTimeInMillisecondsSinceEpoch();

    // Time Trial Countdown
    if (timetrial->LastPlayerResetTime == timetrial->WorldInfo->TimeSeconds) {
        // TODO: Find a way to reset the world's time seconds without side effects
        // The characters mesh gets messed up if you use world->TimeSeconds = 0.0f

        printf("timetrial: countdown started\n");
        ResetValues();
    }

    // Time Trial Started
    if (timetrial->RaceCountDownTimer == 0 && time == 0.0f && distance == 0.0f) {
        printf("timetrial: race started\n");

        startTimeStampUnix = info.unixTime;
        timeTrialRealTimeStarted = timetrial->WorldInfo->RealTimeSeconds;
    }

    // Top Speed
    if (timetrial->RacingPawn && timetrial->RacingPawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) {
        if (speed > timeTrialCheckpointTopSpeedInfo.topspeed) {
            timeTrialCheckpointTopSpeedInfo = info;
        }
    }

    // Respawn
    if (timetrial->RaceCountDownTimer == 0 && timetrial->CurrentTimeData.TotalTime == 0.0f && timetrial->LastPlayerResetTime != oldTimeTrial.LastPlayerResetTime) {
        printf("timetrial: player respawned\n");
        timeTrialCheckpointRespawnInfo.push_back(info);
    }

    // Time Trial Checkpoint Touched
    if (timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) {
        printf("timetrial: checkpoint touched (%d/%d)\n", timetrial->NumPassedCheckPoints, timetrial->NumCheckPoints);

        TimeTrialInfo ttInfo;
        ttInfo.avgspeed = (distance / time) * MS_TO_KPH;
        ttInfo.topSpeedInfo = timeTrialCheckpointTopSpeedInfo;
        ttInfo.respawnInfo = timeTrialCheckpointRespawnInfo;
        ttInfo.timedCheckpoint = timetrial->NumPassedTimerCheckPoints > oldTimeTrial.NumPassedTimerCheckPoints;
        ttInfo.intermediateTime = time;
        ttInfo.accumulatedIntermediateTime = time + timeTrialCheckpointPreviousTime;
        ttInfo.intermediateDistance = distance;
        ttInfo.accumulatedIntermediateDistance = distance + timeTrialCheckpointPreviousDistance;
        ttInfo.checkpointTouchedUnixTime = info.unixTime;

        timeTrialCheckpointInfo.push_back(ttInfo);
        timeTrialCheckpointTopSpeedInfo = {0};
        timeTrialCheckpointRespawnInfo.clear();
        timeTrialCheckpointPreviousTime = time + timeTrialCheckpointPreviousTime;
        timeTrialCheckpointPreviousDistance = distance + timeTrialCheckpointPreviousDistance;
    }

    // Time Trial Finish
    if (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints && timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) {
        printf("timetrial: race finished\n");
        endTimeStampUnix = info.unixTime;

        if (timeTrialCheckpointInfo.size() != timetrial->NumPassedCheckPoints || startTimeStampUnix == std::chrono::milliseconds(0)) {
            printf("timetrial: invalid data!\n");
            printf("- timeTrialCheckpointInfo.size(): %d\n", timeTrialCheckpointInfo.size());
            printf("- timetrial->NumPassedCheckPoints: %d\n", timetrial->NumPassedCheckPoints);
            printf("- startTimeStampUnix: %lld\n\n", startTimeStampUnix.count());
            return;
        }

        json jsonIntermediateDistances;
        json jsonCheckpoints;
        json jsonCheckpointData;

        for (int i = 0; i < timetrial->NumCheckPoints; i++) {
            json jsonRespawn;

            for (size_t j = 0; j < timeTrialCheckpointInfo[i].respawnInfo.size(); j++) {
                jsonRespawn.push_back({
                    {"Time", timeTrialCheckpointInfo[i].respawnInfo[j].time},
                    {"Distance", timeTrialCheckpointInfo[i].respawnInfo[j].distance},
                    {"AvgSpeed", timeTrialCheckpointInfo[i].respawnInfo[j].avgspeed},
                    {"Topspeed", timeTrialCheckpointInfo[i].respawnInfo[j].topspeed},
                    {"IntermediateTime", timeTrialCheckpointInfo[i].respawnInfo[j].intermediateTime},
                    {"IntermediateDistance", timeTrialCheckpointInfo[i].respawnInfo[j].intermediateDistance},
                    {"UnixTime", timeTrialCheckpointInfo[i].respawnInfo[j].unixTime.count()}
                });
		    }

            jsonCheckpoints.push_back({
                {"AvgSpeed", timeTrialCheckpointInfo[i].avgspeed},
                {"RespawnInfo", jsonRespawn},
                {"TopSpeedInfo", {
                    {"Time", timeTrialCheckpointInfo[i].topSpeedInfo.time},
                    {"Distance", timeTrialCheckpointInfo[i].topSpeedInfo.distance},
                    {"AvgSpeed", timeTrialCheckpointInfo[i].topSpeedInfo.avgspeed},
                    {"Topspeed", timeTrialCheckpointInfo[i].topSpeedInfo.topspeed},
                    {"IntermediateTime", timeTrialCheckpointInfo[i].topSpeedInfo.intermediateTime},
                    {"IntermediateDistance", timeTrialCheckpointInfo[i].topSpeedInfo.intermediateDistance},
                    {"UnixTime", timeTrialCheckpointInfo[i].topSpeedInfo.unixTime.count()}}
                },
                {"TimedCheckpoint", timeTrialCheckpointInfo[i].timedCheckpoint == true ? "true" : "false"},
                {"IntermediateTime", timeTrialCheckpointInfo[i].intermediateTime},
                {"AccumulatedIntermediateTime", timeTrialCheckpointInfo[i].accumulatedIntermediateTime},
                {"IntermediateDistance", timeTrialCheckpointInfo[i].intermediateDistance},
                {"AccumulatedIntermediateDistance", timeTrialCheckpointInfo[i].accumulatedIntermediateDistance},
                {"CheckpointTouchedUnixTime", timeTrialCheckpointInfo[i].checkpointTouchedUnixTime.count()}
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

        for (int i = 0; i < timetrial->NumPassedTimerCheckPoints; i++) {
			jsonIntermediateDistances.push_back(timetrial->CurrentTackData.IntermediateDistance[i] / 100.0f);
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", levelName},

            // Time Trial data
            {"TotalTime", timetrial->CurrentTimeData.TotalTime},
            {"TotalTimeRTA", timetrial->WorldInfo->RealTimeSeconds - timeTrialRealTimeStarted},
            {"PlayerDistance", timetrial->PlayerDistance},
            {"AvgSpeed", (timetrial->PlayerDistance / timetrial->CurrentTimeData.TotalTime) * MS_TO_KPH},

            // Custom data
            {"CheckpointData", jsonCheckpoints},

            // These are sent in order to verify that the run is valid
            {"TrackData", {
                {"ActiveTTStretch", timetrial->ActiveTTStretch.GetValue()},
                {"QualifyingTime", timetrial->QualifyingTime},
                {"StarRatingTimes", {
                    {"3", timetrial->StarRatingTimes[0]},
                    {"2", timetrial->StarRatingTimes[1]},
                    {"1", timetrial->StarRatingTimes[2]}}
                },
                {"TotalDistance", timetrial->CurrentTackData.TotalDistance / 100.0f},
                {"IntermediateDistances", jsonIntermediateDistances},
                {"CheckpointData", jsonCheckpointData}}
            }
        };

        SendJsonData(jsonData);
    }
}

static void OnTickSpeedRun(Classes::ATdSPLevelRace* speedrun, const float deltaTime) {
    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();
    const auto checkpointManager = speedrun->TdGameData->CheckpointManager;
    const bool isTimerAtZero = speedrun->TdGameData->TimeAttackClock == 0.0f && pawn->WorldInfo->RealTimeSeconds == 0.0f;
    
    // Info about the current tick such as time, avgspeed, and etc
    Info info;
    info.time = speedrun->TdGameData->TimeAttackClock;
    info.distance = speedrun->TdGameData->TimeAttackDistance;
    info.avgspeed = (info.distance / info.time) * MS_TO_KPH;
    info.topspeed = topSpeed;
    info.worldTimeSeconds = speedrun->WorldInfo->TimeSeconds;
    info.worldRealTimeSeconds = speedrun->WorldInfo->RealTimeSeconds;
    info.checkpointWeight = checkpointManager->ActiveCheckpointWeight;
    info.unixTime = GetTimeInMillisecondsSinceEpoch();

    // Speedrun Restarted Using Binding
    if (isTimerAtZero && speedrun->TdGameData->TimeAttackDistance != 0.0f) {
        printf("speedrun: \"TimeAttackDistance\" is not 0.0f! Setting it to 0.0f. Resetlevel binding used.\n");
        speedrun->TdGameData->TimeAttackDistance = 0.0f;
    }

    // Speedrun Start
    if (isTimerAtZero && speedrun->TdGameData->TimeAttackDistance == 0.0f) {
        printf("speedrun: race started\n");

        ResetValues();
        startTimeStampUnix = info.unixTime;
    }

    // Top Speed
    if (!speedrun->bRaceOver && pawn->bAllowMoveChange) {
        const float speed = sqrtf(powf(pawn->Velocity.X, 2) + powf(pawn->Velocity.Y, 2)) * CMS_TO_KPH;

        if (speed > topSpeed && pawn->Health != 0 && pawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) {
            topSpeed = speed;
        }
    }

    // Reactiontime
    if (controller->bReactionTime && controller->bReactionTime != oldController.bReactionTime && !speedrun->bRaceOver) {
        printf("speedrun: reactiontime used\n");
        speedrunReactionTimeInfo.push_back(info);
    }

    // Health
    if (pawn->Health <= 0 && pawn->Health != oldPawn.Health) {
        printf("speedrun: player died\n");
        speedrunPlayerDeathsInfo.push_back(info);
    }

    // Speedrun Checkpoint Touched
    if (checkpointManager->ActiveCheckpointWeight > oldSpeedrun.ActiveCheckpointWeight && checkpointManager->ActiveCheckpointWeight != 0) {
        printf("speedrun: checkpoint touched (weight: %d)\n", checkpointManager->ActiveCheckpointWeight);

        topSpeed = 0.0f;
        speedrunCheckpointInfo.push_back(info);
    }

    // Speedrun Finish
    if (speedrun->bRaceOver && speedrun->bRaceOver != oldSpeedrun.bRaceOver) {
        printf("speedrun: race finished\n");

        endTimeStampUnix = info.unixTime;

        if (startTimeStampUnix == std::chrono::milliseconds(0)) {
            printf("speedrun: invalid data!\n");
            printf("- startTimeStampUnix: %lld\n\n", startTimeStampUnix.count());
            return;
        }

        json jsonCheckpoints;
        json jsonReactionTimeInfo;
        json jsonPlayerDeathsInfo;

        for (size_t i = 0; i < speedrunCheckpointInfo.size(); i++) {
			jsonCheckpoints.push_back({
                 {"Time", speedrunCheckpointInfo[i].time},
                 {"Distance", speedrunCheckpointInfo[i].distance},
                 {"AvgSpeed", speedrunCheckpointInfo[i].avgspeed},
                 {"TopSpeed", speedrunCheckpointInfo[i].topspeed},
                 {"WorldTimeSeconds", speedrunCheckpointInfo[i].worldTimeSeconds},
                 {"WorldRealTimeSeconds", speedrunCheckpointInfo[i].worldRealTimeSeconds},
                 {"CheckpointWeight", speedrunCheckpointInfo[i].checkpointWeight},
                 {"UnixTime", speedrunCheckpointInfo[i].unixTime.count()}
            });
		}

        for (size_t i = 0; i < speedrunReactionTimeInfo.size(); i++) {
			jsonReactionTimeInfo.push_back({
                 {"Time", speedrunReactionTimeInfo[i].time},
                 {"Distance", speedrunReactionTimeInfo[i].distance},
                 {"AvgSpeed", speedrunReactionTimeInfo[i].avgspeed},
                 {"TopSpeed", speedrunReactionTimeInfo[i].topspeed},
                 {"WorldTimeSeconds", speedrunReactionTimeInfo[i].worldTimeSeconds},
                 {"WorldRealTimeSeconds", speedrunReactionTimeInfo[i].worldRealTimeSeconds},
                 {"CheckpointWeight", speedrunReactionTimeInfo[i].checkpointWeight},
                 {"UnixTime", speedrunReactionTimeInfo[i].unixTime.count()}
            });
		}

        for (size_t i = 0; i < speedrunPlayerDeathsInfo.size(); i++) {
			jsonPlayerDeathsInfo.push_back({
                 {"Time", speedrunPlayerDeathsInfo[i].time},
                 {"Distance", speedrunPlayerDeathsInfo[i].distance},
                 {"AvgSpeed", speedrunPlayerDeathsInfo[i].avgspeed},
                 {"TopSpeed", speedrunPlayerDeathsInfo[i].topspeed},
                 {"WorldTimeSeconds", speedrunPlayerDeathsInfo[i].worldTimeSeconds},
                 {"WorldRealTimeSeconds", speedrunPlayerDeathsInfo[i].worldRealTimeSeconds},
                 {"CheckpointWeight", speedrunPlayerDeathsInfo[i].checkpointWeight},
                 {"UnixTime", speedrunPlayerDeathsInfo[i].unixTime.count()}
            });
		}

        json jsonData = {
            // The current map name in lower case
            {"MapName", levelName},
            
            // Speedrun
            {"TimeAttackClock", speedrun->TdGameData->TimeAttackClock},
            {"TimeAttackDistance", speedrun->TdGameData->TimeAttackDistance},
            
            // AWorldInfo
            {"WorldTimeSeconds", pawn->WorldInfo->TimeSeconds},
            {"WorldRealTimeSeconds", pawn->WorldInfo->RealTimeSeconds},
            
            // Custom data
            {"AvgSpeed", (speedrun->TdGameData->TimeAttackDistance / speedrun->TdGameData->TimeAttackClock) * MS_TO_KPH},
            {"CheckpointInfo", jsonCheckpoints},
            {"ReactionTimeInfo", jsonReactionTimeInfo},
            {"PlayerDeathsInfo", jsonPlayerDeathsInfo}
        };

        SendJsonData(jsonData);
    }
}

static void OnTick(const float deltaTime) {
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
    submitRun = Settings::GetSetting("race", "submitRun", true);
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