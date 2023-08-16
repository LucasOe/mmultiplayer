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

// TODO: use a structure instead and have one variable
static Leaderboard::Old::TimeTrial oldTimeTrial;
static Leaderboard::Old::Speedrun oldSpeedrun;
static Leaderboard::Old::Pawn oldPawn;
static Leaderboard::Old::Controller oldController;

// Info about the current tick such as time, avgspeed, and etc
static InfoStruct info;
static TimeTrialInfoStruct ttInfo;

// Time Trial
static float timeTrialCheckpointPreviousTime = 0.0f;
static float timeTrialCheckpointPreviousDistance = 0.0f;
static std::vector<InfoStruct> timeTrialCheckpointRespawnInfo;
static std::vector<TimeTrialInfoStruct> timeTrialCheckpointInfo;

// Chapter Speedrun
static float speedrunCheckpointPreviousDistance = 0.0f;
static std::vector<InfoStruct> speedrunCheckpointInfo;
static std::vector<InfoStruct> speedrunReactionTimeInfo;
static std::vector<InfoStruct> speedrunPlayerDeathsInfo;

// Timestamps in unix time. These are used to know if the run was started properly or not
static std::chrono::milliseconds UnixTimeStampStart = std::chrono::milliseconds(0);
static std::chrono::milliseconds UnixTimeStampEnd = std::chrono::milliseconds(0);

static float TopSpeed = 0.0f;
static InfoStruct TopSpeedInfo = {0};

static void SaveData(const Classes::ATdPlayerPawn* pawn, const Classes::ATdPlayerController* controller, const Classes::ATdSPTimeTrialGame* timetrial, const Classes::ATdSPLevelRace* speedrun) 
{
    if (pawn) 
    {
        oldPawn.Location = pawn->Location;
        oldPawn.Health = pawn->Health;
        oldPawn.bSRPauseTimer = pawn->bSRPauseTimer;
    }

    if (controller) 
    {
        oldController.bReactionTime = controller->bReactionTime;
        oldController.ReactionTimeEnergy = controller->ReactionTimeEnergy;
    }

    if (speedrun) 
    {
        oldSpeedrun.bRaceOver = speedrun->bRaceOver;
        oldSpeedrun.TimeAttackClock = speedrun->TdGameData->TimeAttackClock;
        oldSpeedrun.TimeAttackDistance = speedrun->TdGameData->TimeAttackDistance;
        oldSpeedrun.ActiveCheckpointWeight = speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight;
    }

    if (timetrial) 
    {
        oldTimeTrial.RaceFinishLineTime = timetrial->RaceFinishLineTime;
        oldTimeTrial.RaceFinishLineTimer = timetrial->RaceFinishLineTimer;
        oldTimeTrial.RaceCountDownTime = timetrial->RaceCountDownTime;
        oldTimeTrial.RaceCountDownTimer = timetrial->RaceCountDownTimer;
        oldTimeTrial.NumCheckPoints = timetrial->NumCheckPoints;
        oldTimeTrial.NumPassedCheckPoints = timetrial->NumPassedCheckPoints;
        oldTimeTrial.NumPassedTimerCheckPoints = timetrial->NumPassedTimerCheckPoints;
        oldTimeTrial.LastCheckpointTimeStamp = timetrial->LastCheckpointTimeStamp;
        oldTimeTrial.LastPlayerResetTime = timetrial->LastPlayerResetTime;
        oldTimeTrial.bDelayPauseUntilRaceStarted = timetrial->bDelayPauseUntilRaceStarted;
        oldTimeTrial.CurrentTackData = timetrial->CurrentTackData;
        oldTimeTrial.CurrentTimeData = timetrial->CurrentTimeData;
        oldTimeTrial.TimeDataToBeat = timetrial->TimeDataToBeat;
        oldTimeTrial.RaceStartTimeStamp = timetrial->RaceStartTimeStamp;
        oldTimeTrial.RaceEndTimeStamp = timetrial->RaceEndTimeStamp;
        oldTimeTrial.PlayerDistance = timetrial->PlayerDistance;
    }
}

static void ResetData() 
{
    timeTrialCheckpointPreviousTime = 0.0f;
    timeTrialCheckpointPreviousDistance = 0.0f;
    timeTrialCheckpointInfo.clear();
    timeTrialCheckpointRespawnInfo.clear();

    speedrunCheckpointInfo.clear();
    speedrunReactionTimeInfo.clear();
    speedrunPlayerDeathsInfo.clear();
    speedrunCheckpointPreviousDistance = 0.0f;

    info = {0};
    ttInfo = {0};

    UnixTimeStampStart = std::chrono::milliseconds(0);
    UnixTimeStampEnd = std::chrono::milliseconds(0);

    TopSpeed = 0.0f;
    TopSpeedInfo = {0};
}

static void LeaderboardTab() 
{ 
    if (ImGui::Checkbox("Enabled", &enabled)) 
    {
        Settings::SetSetting("race", "enabled", enabled);
    }

    if (!enabled) 
    {
        return;
    }

    if (ImGui::Checkbox("Submit Run", &submitRun)) 
    {
        Settings::SetSetting("race", "submitRun", submitRun);
    }

    ImGui::SeperatorWithPadding(2.5f);

    ImGui::Text("Name");
    if (ImGui::InputText("##leaderboard-name-input", nameInput, sizeof(nameInput))) 
    {
        if (playerName != nameInput) 
        {
            bool empty = true;
            for (auto c : std::string(nameInput)) 
            {
                if (!isblank(c)) 
                {
                    empty = false;
                    break;
                }
            }

            Settings::SetSetting("race", "playerName", playerName = (!empty ? nameInput : ""));
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (!playerName.empty() && submitRun) 
    {
        ImGui::Text("Your runs that you finish will be uploaded to a server");
    }
    else 
    {
        ImGui::Text("Your runs that you finish will >> NOT << be uploaded to a server");
        if (playerName.empty()) 
        {
            ImGui::Text("- The name can't be empty!");
        } 
        else 
        {
            ImGui::Text("- \"Submit Run\" is off");
        }
    }
}

static void SendJsonData(json jsonData) 
{
    jsonData.push_back({"Username", playerName});
    jsonData.push_back({"StartTimeStampUnix", UnixTimeStampStart.count()});
    jsonData.push_back({"EndTimeStampUnix", UnixTimeStampEnd.count()});

    if (!playerName.empty() && submitRun) 
    {
        json json = {
            {"type", "post"},
            {"body", jsonData.dump()}
        };

        Client client;
        if (client.SendJsonMessage(json)) 
        {
            printf("data sent successfully! :D\n\n");
        }
        else 
        {
            printf("data failed to send! Multiplayer disabled or no internet connection?\n\n");
        }
    } 
    else 
    {
        printf("data failed to send. Check leaderboard tab to see if it can send it or not\n\n");
    }

    ResetData();
}

static std::chrono::milliseconds GetTimeInMillisecondsSinceEpoch() 
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

static json VectorInfoToJson(const std::vector<InfoStruct> info) {
    json json;

    for (size_t i = 0; i < info.size(); i++) 
    {
		json.push_back({
            {"Time", info[i].Time},
            {"Distance", info[i].Distance},
            {"AvgSpeed", info[i].AvgSpeed},
            {"TopSpeed", info[i].TopSpeed},
            {"WorldTimeSeconds", info[i].WorldTimeSeconds},
            {"WorldRealTimeSeconds", info[i].WorldRealTimeSeconds},
            {"CheckpointWeight", info[i].CheckpointWeight},
            {"PlayerLocation", {
                {"X", info[i].Location.X / 100.0f},
                {"Y", info[i].Location.Y / 100.0f},
                {"Z", info[i].Location.Z / 100.0f}
            }},
            {"UnixTime", info[i].UnixTime.count()}
        });
	}

    return json;
}

static void OnTickTimeTrial(Classes::ATdSPTimeTrialGame* timetrial) 
{
    const float distance = (timetrial->PlayerDistance / (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints ? 1 : 100)) - timeTrialCheckpointPreviousDistance;
    const float time = timetrial->WorldInfo->TimeSeconds - (timetrial->RaceStartTimeStamp != -1.0f ? timetrial->RaceStartTimeStamp : -1.0f) - timeTrialCheckpointPreviousTime;
    const float speed = timetrial->RacingPawn ? sqrtf(powf(timetrial->RacingPawn->Velocity.X, 2) + powf(timetrial->RacingPawn->Velocity.Y, 2)) * CMS_TO_KPH : 0.0f;

    info.Time = time + timeTrialCheckpointPreviousTime;
    info.Distance = distance + timeTrialCheckpointPreviousDistance;
    info.AvgSpeed = (info.Distance / info.Time) * MS_TO_KPH;
    info.TopSpeed = TopSpeed;
    info.IntermediateTime = time;
    info.IntermediateDistance = distance;
    info.Location = oldPawn.Location;
    info.UnixTime = GetTimeInMillisecondsSinceEpoch();

    // Time Trial Countdown 
    if (timetrial->RaceCountDownTimer == timetrial->RaceCountDownTime + 1 && timetrial->LastPlayerResetTime == timetrial->WorldInfo->TimeSeconds) 
    {
        printf("timetrial: countdown started\n");

        // TODO: Find a way to reset the world's time seconds without side effects
        // The characters mesh gets messed up if you use world->TimeSeconds = 0.0f

        ResetData();
    }

    // Time Trial Start
    if (timetrial->RaceCountDownTimer == 0 && oldTimeTrial.RaceCountDownTimer == 1) 
    {
        printf("timetrial: race started\n");
        UnixTimeStampStart = info.UnixTime;
    }

    // Top Speed
    if (timetrial->RacingPawn && timetrial->RacingPawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) 
    {
        if (speed > TopSpeed) 
        {
            TopSpeed = speed;
            TopSpeedInfo = info;
        }
    }

    // Respawn
    if (timetrial->RaceCountDownTimer == 0 && timetrial->LastPlayerResetTime != oldTimeTrial.LastPlayerResetTime) 
    {
        printf("timetrial: player respawned\n");
        timeTrialCheckpointRespawnInfo.push_back(info);
    }

    // Time Trial Checkpoint Touched
    if (timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) 
    {
        printf("timetrial: checkpoint touched (%d/%d)\n", timetrial->NumPassedCheckPoints, timetrial->NumCheckPoints);

        ttInfo.AvgSpeed = info.AvgSpeed;
        ttInfo.TimedCheckpoint = timetrial->NumPassedTimerCheckPoints > oldTimeTrial.NumPassedTimerCheckPoints;
        ttInfo.IntermediateTime = time;
        ttInfo.AccumulatedIntermediateTime = time + timeTrialCheckpointPreviousTime;
        ttInfo.IntermediateDistance = distance;
        ttInfo.AccumulatedIntermediateDistance = distance + timeTrialCheckpointPreviousDistance;
        ttInfo.RespawnInfo = timeTrialCheckpointRespawnInfo;
        ttInfo.TopSpeedInfo = TopSpeedInfo;
        ttInfo.Location = info.Location;
        ttInfo.UnixTime = info.UnixTime;

        TopSpeed = 0.0f;
        TopSpeedInfo = {0};

        timeTrialCheckpointInfo.push_back(ttInfo);
        timeTrialCheckpointRespawnInfo.clear();
        timeTrialCheckpointPreviousTime = time + timeTrialCheckpointPreviousTime;
        timeTrialCheckpointPreviousDistance = distance + timeTrialCheckpointPreviousDistance;
    }

    // Time Trial Finish
    if (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints && timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) 
    {
        printf("timetrial: race finished\n");
        UnixTimeStampEnd = info.UnixTime;

        if (timeTrialCheckpointInfo.size() != timetrial->NumPassedCheckPoints || UnixTimeStampStart == std::chrono::milliseconds(0)) 
        {
            printf("timetrial: invalid data! :(\n");
            printf("- timeTrialCheckpointInfo.size(): %d\n", timeTrialCheckpointInfo.size());
            printf("- timetrial->NumPassedCheckPoints: %d\n", timetrial->NumPassedCheckPoints);
            printf("- UnixTimeStampStart: %lld\n\n", UnixTimeStampStart.count());
            return;
        }

        json jsonIntermediateDistances;
        json jsonCheckpoints;
        json jsonCheckpointData;

        for (int i = 0; i < timetrial->NumCheckPoints; i++) 
        {
            json jsonRespawns;

            for (size_t j = 0; j < timeTrialCheckpointInfo[i].RespawnInfo.size(); j++) 
            {
		    	jsonRespawns.push_back({
                    {"Time", timeTrialCheckpointInfo[i].RespawnInfo[j].Time},
                    {"Distance", timeTrialCheckpointInfo[i].RespawnInfo[j].Distance},
                    {"AvgSpeed", timeTrialCheckpointInfo[i].RespawnInfo[j].AvgSpeed},
                    {"TopSpeed", timeTrialCheckpointInfo[i].RespawnInfo[j].TopSpeed},
                    {"IntermediateTime", timeTrialCheckpointInfo[i].RespawnInfo[j].IntermediateTime},
                    {"IntermediateDistance", timeTrialCheckpointInfo[i].RespawnInfo[j].IntermediateDistance},
                    {"Location", {
                        {"X", timeTrialCheckpointInfo[i].RespawnInfo[j].Location.X / 100.0f},
                        {"Y", timeTrialCheckpointInfo[i].RespawnInfo[j].Location.Y / 100.0f},
                        {"Z", timeTrialCheckpointInfo[i].RespawnInfo[j].Location.Z / 100.0f}
                    }},
                    {"UnixTime", timeTrialCheckpointInfo[i].RespawnInfo[j].UnixTime.count()}
                });
		    }

            jsonCheckpoints.push_back({
                {"AvgSpeed", timeTrialCheckpointInfo[i].AvgSpeed},
                {"TimedCheckpoint", timeTrialCheckpointInfo[i].TimedCheckpoint},
                {"IntermediateTime", timeTrialCheckpointInfo[i].IntermediateTime},
                {"AccumulatedIntermediateTime", timeTrialCheckpointInfo[i].AccumulatedIntermediateTime},
                {"IntermediateDistance", timeTrialCheckpointInfo[i].IntermediateDistance},
                {"AccumulatedIntermediateDistance", timeTrialCheckpointInfo[i].AccumulatedIntermediateDistance},
                {"UnixTime", timeTrialCheckpointInfo[i].UnixTime.count()},
                {"RespawnInfo", jsonRespawns},
                {"TopSpeedInfo", {
                    {"Time", timeTrialCheckpointInfo[i].TopSpeedInfo.Time},
                    {"Distance", timeTrialCheckpointInfo[i].TopSpeedInfo.Distance},
                    {"AvgSpeed", timeTrialCheckpointInfo[i].TopSpeedInfo.AvgSpeed},
                    {"TopSpeed", timeTrialCheckpointInfo[i].TopSpeedInfo.TopSpeed},
                    {"IntermediateTime", timeTrialCheckpointInfo[i].TopSpeedInfo.IntermediateTime},
                    {"IntermediateDistance", timeTrialCheckpointInfo[i].TopSpeedInfo.IntermediateDistance},
                    {"Location", {
                        {"X", timeTrialCheckpointInfo[i].TopSpeedInfo.Location.X / 100.0f},
                        {"Y", timeTrialCheckpointInfo[i].TopSpeedInfo.Location.Y / 100.0f},
                        {"Z", timeTrialCheckpointInfo[i].TopSpeedInfo.Location.Z / 100.0f}
                    }},
                    {"UnixTime", timeTrialCheckpointInfo[i].TopSpeedInfo.UnixTime.count()}
                }}
            });
        }

        for (int i = 0; i < timetrial->NumPassedCheckPoints; i++) 
        {
            const auto track = static_cast<Classes::ATdTimerCheckpoint*>(timetrial->CheckpointManager->ActiveTrack[i]);

            json jsonBelongToTracks;

            for (size_t j = 0; j < track->BelongToTracks.Num(); j++) 
            {
			    jsonBelongToTracks.push_back({
                    {"TrackIndex", track->BelongToTracks[j].TrackIndex.GetValue()},
                    {"OrderIndex", track->BelongToTracks[j].OrderIndex},
                    {"bNoIntermediateTime", track->BelongToTracks[j].bNoIntermediateTime == 1 ? true : false}
                });
		    }

            jsonCheckpointData.push_back({
                // UObject
                {"FullName", track->GetFullName()},

                // AActor
                {"X", track->Location.X / 100.0f},
                {"Y", track->Location.Y / 100.0f},
                {"Z", track->Location.Z / 100.0f},

                // ATdTimerCheckpoint
                {"BelongToTracks", jsonBelongToTracks},
                {"CustomHeight", track->CustomHeight},
                {"CustomWidthScale", track->CustomWidthScale},
                {"bNoRespawn", track->bNoRespawn == 1 ? true : false},
                {"InitialHeight", track->InitialHeight},
                {"InitialRadius", track->InitialRadius},

                // ATdPlaceableCheckpoint
                {"bShouldBeBased", track->bShouldBeBased == 1 ? true : false},
            });
        }

        for (int i = 0; i < timetrial->NumPassedTimerCheckPoints; i++) 
        {
			jsonIntermediateDistances.push_back(timetrial->CurrentTackData.IntermediateDistance[i] / 100.0f);
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", levelName},

            // Time Trial data
            {"TotalTime", timetrial->CurrentTimeData.TotalTime},
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

static void OnTickSpeedRun(Classes::ATdSPLevelRace* speedrun) 
{
    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();
    
    info.Time = speedrun->TdGameData->TimeAttackClock;
    info.Distance = speedrun->TdGameData->TimeAttackDistance;
    info.AvgSpeed = (info.Distance / info.Time) * MS_TO_KPH;
    info.TopSpeed = TopSpeed;
    info.WorldTimeSeconds = speedrun->WorldInfo->TimeSeconds;
    info.WorldRealTimeSeconds = speedrun->WorldInfo->RealTimeSeconds;
    info.CheckpointWeight = speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight;
    info.Location = oldPawn.Location; 
    info.UnixTime = GetTimeInMillisecondsSinceEpoch();

    // Speedrun Restarted Using Binding
    if (pawn->WorldInfo->RealTimeSeconds == 0.0f && speedrun->TdGameData->TimeAttackClock == 0.0f && speedrun->TdGameData->TimeAttackDistance != 0.0f) 
    {
        printf("speedrun: \"TimeAttackDistance\" is not 0.0f! Setting it to 0.0f\n");
        speedrun->TdGameData->TimeAttackDistance = 0.0f;
    }

    // Speedrun Start
    if (pawn->WorldInfo->RealTimeSeconds == 0.0f && speedrun->TdGameData->TimeAttackClock == 0.0f && speedrun->TdGameData->TimeAttackDistance == 0.0f) 
    {
        printf("speedrun: race started\n");

        ResetData();
        UnixTimeStampStart = GetTimeInMillisecondsSinceEpoch();
    }

    // Top Speed
    if (pawn->bAllowMoveChange && pawn->Health > 0) 
    {
        const float speed = sqrtf(powf(pawn->Velocity.X, 2) + powf(pawn->Velocity.Y, 2)) * CMS_TO_KPH;

        if (speed > TopSpeed && pawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) 
        {
            TopSpeed = speed;
        }
    }

    // ReactionTime
    if (controller->bReactionTime && controller->bReactionTime != oldController.bReactionTime) 
    {
        printf("speedrun: reactiontime used\n");
        speedrunReactionTimeInfo.push_back(info);
    }

    // Health
    if (pawn->Health <= 0 && pawn->Health != oldPawn.Health) 
    {
        printf("speedrun: player died\n");
        speedrunPlayerDeathsInfo.push_back(info);
    }

    // Speedrun Checkpoint Touched
    if (speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight > oldSpeedrun.ActiveCheckpointWeight && speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight != 0 && speedrun->TdGameData->TimeAttackDistance != speedrunCheckpointPreviousDistance) 
    {
        printf("speedrun: checkpoint touched (weight: %d)\n", speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight);

        TopSpeed = 0.0f;
        speedrunCheckpointInfo.push_back(info);
        speedrunCheckpointPreviousDistance = speedrun->TdGameData->TimeAttackDistance;
    }

    // Speedrun Finish
    if (speedrun->bRaceOver && speedrun->bRaceOver != oldSpeedrun.bRaceOver) 
    {
        printf("speedrun: race finished\n");
        UnixTimeStampEnd = info.UnixTime;

        // If this is zero, that means that multiplayer was launched or leaderboard got toggled on during the race and therefor is invalid
        if (UnixTimeStampStart == std::chrono::milliseconds(0)) 
        {
            printf("speedrun: invalid data! :(\n- UnixTimeStampStart: %lld\n\n", UnixTimeStampStart.count());
            return;
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", levelName},
            
            // Speedrun
            {"TimeAttackClock", speedrun->TdGameData->TimeAttackClock},
            {"TimeAttackDistance", speedrun->TdGameData->TimeAttackDistance},
            
            // World Info Time
            {"WorldTimeSeconds", pawn->WorldInfo->TimeSeconds},
            {"WorldRealTimeSeconds", pawn->WorldInfo->RealTimeSeconds},
            
            // Custom data
            {"AvgSpeed", (speedrun->TdGameData->TimeAttackDistance / speedrun->TdGameData->TimeAttackClock) * MS_TO_KPH},
            {"CheckpointInfo", VectorInfoToJson(speedrunCheckpointInfo)},
            {"ReactionTimeInfo", VectorInfoToJson(speedrunReactionTimeInfo)},
            {"PlayerDeathsInfo", VectorInfoToJson(speedrunPlayerDeathsInfo)}
        };

        SendJsonData(jsonData);
    }
}

static void OnTick(const float deltaTime) 
{
    if (!enabled || playerName.empty() || levelName.empty() || levelName == Map_MainMenu) 
    {
        return;
    }

    const auto world = Engine::GetWorld();
    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();

    if (!world || !pawn || !controller) 
    {
        return;
    }

    if (world) 
    {
        std::string game = world->Game->GetName();

        if (game.find("TdSPTimeTrialGame") != -1) 
        {
            auto timetrial = static_cast<Classes::ATdSPTimeTrialGame*>(world->Game);

            OnTickTimeTrial(timetrial);
            SaveData(pawn, controller, timetrial, nullptr);
        } 
        else if (game.find("TdSPLevelRace") != -1) 
        {
            auto speedrun = static_cast<Classes::ATdSPLevelRace*>(world->Game);

            OnTickSpeedRun(speedrun);
            SaveData(pawn, controller, nullptr, speedrun);
        }
    }
}

bool Leaderboard::Initialize() 
{
    enabled = Settings::GetSetting("race", "enabled", false);
    submitRun = Settings::GetSetting("race", "submitRun", true);
    playerName = Settings::GetSetting("race", "playerName", "").get<std::string>();
    strncpy_s(nameInput, sizeof(nameInput) - 1, playerName.c_str(), sizeof(nameInput) - 1);

	Menu::AddTab("Leaderboard", LeaderboardTab);
    Engine::OnTick(OnTick);

	Engine::OnPreLevelLoad([](const wchar_t *levelNameW) 
    {
		levelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(levelNameW);

		std::transform(levelName.begin(), levelName.end(), levelName.begin(), [](char c) 
        { 
			return tolower(c); 
		});
	});

	return true;
}

std::string Leaderboard::GetName() 
{
    return "Leaderboard"; 
}