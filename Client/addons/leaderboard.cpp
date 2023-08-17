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

static bool Enabled = false;
static bool SubmitRun = true;
static char NameInput[0x20] = {0};
static std::string PlayerName;
static std::string LevelName;

static OldStruct Old;

// Info about the current tick such as time, avgspeed, and etc
static InfoStruct Info;
static TimeTrialInfoStruct TimeTrialInfo;

// Time Trial
static std::vector<InfoStruct> TimeTrialCheckpointRespawnInfo;
static std::vector<TimeTrialInfoStruct> TimeTrialCheckpointInfo;

// Chapter Speedrun
static std::vector<InfoStruct> SpeedrunCheckpointInfo;
static std::vector<InfoStruct> SpeedrunReactionTimeInfo;
static std::vector<InfoStruct> SpeedrunPlayerDeathsInfo;

// Timestamps in unix time. These are used to know if the run was started properly or not
static std::chrono::milliseconds UnixTimeStampStart = std::chrono::milliseconds(0);
static std::chrono::milliseconds UnixTimeStampEnd = std::chrono::milliseconds(0);

static float TopSpeed = 0.0f;
static InfoStruct TopSpeedInfo = {0};

static float PreviousTime = 0.0f;
static float PreviousDistance = 0.0f;

static void SaveData(const Classes::ATdPlayerPawn* pawn, const Classes::ATdPlayerController* controller, const Classes::ATdSPTimeTrialGame* timetrial, const Classes::ATdSPLevelRace* speedrun)
{
    if (pawn)
    {
        Old.Location = pawn->Location;
        Old.Health = pawn->Health;
        Old.bSRPauseTimer = pawn->bSRPauseTimer;
    }

    if (controller)
    {
        Old.bReactionTime = controller->bReactionTime;
        Old.ReactionTimeEnergy = controller->ReactionTimeEnergy;
    }

    if (speedrun)
    {
        Old.bRaceOver = speedrun->bRaceOver;
        Old.TimeAttackClock = speedrun->TdGameData->TimeAttackClock;
        Old.TimeAttackDistance = speedrun->TdGameData->TimeAttackDistance;
        Old.ActiveCheckpointWeight = speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight;
    }

    if (timetrial)
    {
        Old.RaceFinishLineTime = timetrial->RaceFinishLineTime;
        Old.RaceFinishLineTimer = timetrial->RaceFinishLineTimer;
        Old.RaceCountDownTime = timetrial->RaceCountDownTime;
        Old.RaceCountDownTimer = timetrial->RaceCountDownTimer;
        Old.NumCheckPoints = timetrial->NumCheckPoints;
        Old.NumPassedCheckPoints = timetrial->NumPassedCheckPoints;
        Old.NumPassedTimerCheckPoints = timetrial->NumPassedTimerCheckPoints;
        Old.LastCheckpointTimeStamp = timetrial->LastCheckpointTimeStamp;
        Old.LastPlayerResetTime = timetrial->LastPlayerResetTime;
        Old.bDelayPauseUntilRaceStarted = timetrial->bDelayPauseUntilRaceStarted;
        Old.CurrentTackData = timetrial->CurrentTackData;
        Old.CurrentTimeData = timetrial->CurrentTimeData;
        Old.TimeDataToBeat = timetrial->TimeDataToBeat;
        Old.RaceStartTimeStamp = timetrial->RaceStartTimeStamp;
        Old.RaceEndTimeStamp = timetrial->RaceEndTimeStamp;
        Old.PlayerDistance = timetrial->PlayerDistance;
    }
}

static void ResetData()
{
    TimeTrialCheckpointInfo.clear();
    TimeTrialCheckpointRespawnInfo.clear();

    SpeedrunCheckpointInfo.clear();
    SpeedrunReactionTimeInfo.clear();
    SpeedrunPlayerDeathsInfo.clear();

    Info = {0};
    TimeTrialInfo = {0};

    UnixTimeStampStart = std::chrono::milliseconds(0);
    UnixTimeStampEnd = std::chrono::milliseconds(0);

    TopSpeed = 0.0f;
    TopSpeedInfo = {0};

    PreviousTime = 0.0f;
    PreviousDistance = 0.0f;
}

static void LeaderboardTab()
{
    if (ImGui::Checkbox("Enabled", &Enabled))
    {
        Settings::SetSetting("race", "enabled", Enabled);
    }

    if (!Enabled)
    {
        return;
    }

    if (ImGui::Checkbox("Submit Run", &SubmitRun))
    {
        Settings::SetSetting("race", "submitRun", SubmitRun);
    }

    ImGui::SeperatorWithPadding(2.5f);

    ImGui::Text("Name");
    if (ImGui::InputText("##leaderboard-name-input", NameInput, sizeof(NameInput)))
    {
        if (PlayerName != NameInput)
        {
            bool empty = true;
            for (auto c : std::string(NameInput))
            {
                if (!isblank(c))
                {
                    empty = false;
                    break;
                }
            }

            Settings::SetSetting("race", "playerName", PlayerName = (!empty ? NameInput : ""));
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (!PlayerName.empty() && SubmitRun)
    {
        ImGui::Text("Your runs that you finish will be uploaded to a server");
    }
    else
    {
        ImGui::Text("Your runs that you finish will >> NOT << be uploaded to a server");
        if (PlayerName.empty())
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
    jsonData.push_back({"Username", PlayerName});
    jsonData.push_back({"StartTimeStampUnix", UnixTimeStampStart.count()});
    jsonData.push_back({"EndTimeStampUnix", UnixTimeStampEnd.count()});

    if (!PlayerName.empty() && SubmitRun)
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

static json VectorInfoToJson(const std::vector<InfoStruct> info)
{
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
                {"Z", info[i].Location.Z / 100.0f}}
            },
            {"UnixTime", info[i].UnixTime.count()},
        });
    }

    return json;
}

static void OnTickTimeTrial(Classes::ATdSPTimeTrialGame* timetrial)
{
    const float distance = (timetrial->PlayerDistance / (timetrial->NumPassedCheckPoints == Old.NumCheckPoints ? 1 : 100)) - PreviousDistance;
    const float time = timetrial->WorldInfo->TimeSeconds - (timetrial->RaceStartTimeStamp != -1.0f ? timetrial->RaceStartTimeStamp : -1.0f) - PreviousTime;
    const float speed = timetrial->RacingPawn ? sqrtf(powf(timetrial->RacingPawn->Velocity.X, 2) + powf(timetrial->RacingPawn->Velocity.Y, 2)) * CMS_TO_KPH : 0.0f;

    Info.Time = time + PreviousTime;
    Info.Distance = distance + PreviousDistance;
    Info.AvgSpeed = (Info.Distance / Info.Time) * MS_TO_KPH;
    Info.TopSpeed = TopSpeed;
    Info.IntermediateTime = time;
    Info.IntermediateDistance = distance;
    Info.Location = Old.Location;
    Info.UnixTime = GetTimeInMillisecondsSinceEpoch();

    // Time Trial Countdown
    if (timetrial->RaceCountDownTimer == timetrial->RaceCountDownTime + 1 && timetrial->LastPlayerResetTime == timetrial->WorldInfo->TimeSeconds)
    {
        printf("timetrial: countdown started\n");

        // TODO: Find a way to reset the world's time seconds without side effects
        // The characters mesh gets messed up if you use world->TimeSeconds = 0.0f

        ResetData();
    }

    // Time Trial Start
    if (timetrial->RaceCountDownTimer == 0 && Old.RaceCountDownTimer == 1)
    {
        printf("timetrial: race started\n");
        UnixTimeStampStart = Info.UnixTime;
    }

    // Top Speed
    if (timetrial->RacingPawn && timetrial->RacingPawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled)
    {
        if (speed > TopSpeed)
        {
            TopSpeed = speed;
            TopSpeedInfo = Info;
        }
    }

    // Respawn
    if (timetrial->RaceCountDownTimer == 0 && timetrial->LastPlayerResetTime != Old.LastPlayerResetTime)
    {
        printf("timetrial: player respawned\n");
        TimeTrialCheckpointRespawnInfo.push_back(Info);
    }

    // Time Trial Checkpoint Touched
    if (timetrial->NumPassedCheckPoints > Old.NumPassedCheckPoints)
    {
        printf("timetrial: checkpoint touched (%d/%d)\n", timetrial->NumPassedCheckPoints, timetrial->NumCheckPoints);

        TimeTrialInfo.AvgSpeed = Info.AvgSpeed;
        TimeTrialInfo.TimedCheckpoint = timetrial->NumPassedTimerCheckPoints > Old.NumPassedTimerCheckPoints;
        TimeTrialInfo.IntermediateTime = time;
        TimeTrialInfo.AccumulatedIntermediateTime = time + PreviousTime;
        TimeTrialInfo.IntermediateDistance = distance;
        TimeTrialInfo.AccumulatedIntermediateDistance = distance + PreviousDistance;
        TimeTrialInfo.RespawnInfo = TimeTrialCheckpointRespawnInfo;
        TimeTrialInfo.TopSpeedInfo = TopSpeedInfo;
        TimeTrialInfo.Location = Info.Location;
        TimeTrialInfo.UnixTime = Info.UnixTime;

        TopSpeed = 0.0f;
        TopSpeedInfo = {0};

        PreviousTime = time + PreviousTime;
        PreviousDistance = distance + PreviousDistance;

        TimeTrialCheckpointRespawnInfo.clear();
        TimeTrialCheckpointInfo.push_back(TimeTrialInfo);
    }

    // Time Trial Finish
    if (timetrial->NumPassedCheckPoints == Old.NumCheckPoints && timetrial->NumPassedCheckPoints > Old.NumPassedCheckPoints)
    {
        printf("timetrial: race finished\n");
        UnixTimeStampEnd = Info.UnixTime;

        if (TimeTrialCheckpointInfo.size() != timetrial->NumPassedCheckPoints || UnixTimeStampStart == std::chrono::milliseconds(0))
        {
            printf("timetrial: invalid data! :(\n");
            printf("- timetrial->NumPassedCheckPoints: %d\n", timetrial->NumPassedCheckPoints);
            printf("- TimeTrialCheckpointInfo.size(): %d\n", TimeTrialCheckpointInfo.size());
            printf("- UnixTimeStampStart: %lld\n\n", UnixTimeStampStart.count());
            return;
        }

        json jsonIntermediateDistances;
        json jsonCheckpoints;
        json jsonCheckpointData;

        for (int i = 0; i < timetrial->NumCheckPoints; i++)
        {
            json jsonRespawns;

            for (size_t j = 0; j < TimeTrialCheckpointInfo[i].RespawnInfo.size(); j++)
            {
                jsonRespawns.push_back({
                    {"Time", TimeTrialCheckpointInfo[i].RespawnInfo[j].Time},
                    {"Distance", TimeTrialCheckpointInfo[i].RespawnInfo[j].Distance},
                    {"AvgSpeed", TimeTrialCheckpointInfo[i].RespawnInfo[j].AvgSpeed},
                    {"TopSpeed", TimeTrialCheckpointInfo[i].RespawnInfo[j].TopSpeed},
                    {"IntermediateTime", TimeTrialCheckpointInfo[i].RespawnInfo[j].IntermediateTime},
                    {"IntermediateDistance", TimeTrialCheckpointInfo[i].RespawnInfo[j].IntermediateDistance},
                    {"Location", {
                        {"X", TimeTrialCheckpointInfo[i].RespawnInfo[j].Location.X / 100.0f}, 
                        {"Y", TimeTrialCheckpointInfo[i].RespawnInfo[j].Location.Y / 100.0f}, 
                        {"Z", TimeTrialCheckpointInfo[i].RespawnInfo[j].Location.Z / 100.0f}}
                    },
                    {"UnixTime", TimeTrialCheckpointInfo[i].RespawnInfo[j].UnixTime.count()}
                });
            }

            jsonCheckpoints.push_back({
                {"AvgSpeed", TimeTrialCheckpointInfo[i].AvgSpeed},
                {"TimedCheckpoint", TimeTrialCheckpointInfo[i].TimedCheckpoint},
                {"IntermediateTime", TimeTrialCheckpointInfo[i].IntermediateTime},
                {"AccumulatedIntermediateTime", TimeTrialCheckpointInfo[i].AccumulatedIntermediateTime},
                {"IntermediateDistance", TimeTrialCheckpointInfo[i].IntermediateDistance},
                {"AccumulatedIntermediateDistance", TimeTrialCheckpointInfo[i].AccumulatedIntermediateDistance},
                {"UnixTime", TimeTrialCheckpointInfo[i].UnixTime.count()},
                {"RespawnInfo", jsonRespawns},
                {"TopSpeedInfo", {
                    {"Time", TimeTrialCheckpointInfo[i].TopSpeedInfo.Time},
                    {"Distance", TimeTrialCheckpointInfo[i].TopSpeedInfo.Distance},
                    {"AvgSpeed", TimeTrialCheckpointInfo[i].TopSpeedInfo.AvgSpeed},
                    {"TopSpeed", TimeTrialCheckpointInfo[i].TopSpeedInfo.TopSpeed},
                    {"IntermediateTime", TimeTrialCheckpointInfo[i].TopSpeedInfo.IntermediateTime},
                    {"IntermediateDistance", TimeTrialCheckpointInfo[i].TopSpeedInfo.IntermediateDistance},
                    {"Location", {
                        {"X", TimeTrialCheckpointInfo[i].TopSpeedInfo.Location.X / 100.0f},
                        {"Y", TimeTrialCheckpointInfo[i].TopSpeedInfo.Location.Y / 100.0f},
                        {"Z", TimeTrialCheckpointInfo[i].TopSpeedInfo.Location.Z / 100.0f}
                    }},
                    {"UnixTime", TimeTrialCheckpointInfo[i].TopSpeedInfo.UnixTime.count()}
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
            {"MapName", LevelName},

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

    Info.Time = speedrun->TdGameData->TimeAttackClock;
    Info.Distance = speedrun->TdGameData->TimeAttackDistance;
    Info.AvgSpeed = (Info.Distance / Info.Time) * MS_TO_KPH;
    Info.TopSpeed = TopSpeed;
    Info.WorldTimeSeconds = speedrun->WorldInfo->TimeSeconds;
    Info.WorldRealTimeSeconds = speedrun->WorldInfo->RealTimeSeconds;
    Info.CheckpointWeight = speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight;
    Info.Location = Old.Location;
    Info.UnixTime = GetTimeInMillisecondsSinceEpoch();

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
    if (controller->bReactionTime && controller->bReactionTime != Old.bReactionTime)
    {
        printf("speedrun: reactiontime used\n");
        SpeedrunReactionTimeInfo.push_back(Info);
    }

    // Health
    if (pawn->Health <= 0 && pawn->Health != Old.Health)
    {
        printf("speedrun: player died\n");
        SpeedrunPlayerDeathsInfo.push_back(Info);
    }

    // Speedrun Checkpoint Touched
    if (speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight > Old.ActiveCheckpointWeight && speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight != 0 && speedrun->TdGameData->TimeAttackDistance != PreviousDistance)
    {
        printf("speedrun: checkpoint touched (weight: %d)\n", speedrun->TdGameData->CheckpointManager->ActiveCheckpointWeight);

        TopSpeed = 0.0f;
        SpeedrunCheckpointInfo.push_back(Info);
        PreviousDistance = speedrun->TdGameData->TimeAttackDistance;
    }

    // Speedrun Finish
    if (speedrun->bRaceOver && speedrun->bRaceOver != Old.bRaceOver)
    {
        printf("speedrun: race finished\n");
        UnixTimeStampEnd = Info.UnixTime;

        // If this is zero, that means that multiplayer was launched or leaderboard got toggled on during the race and therefore is invalid
        if (UnixTimeStampStart == std::chrono::milliseconds(0))
        {
            printf("speedrun: invalid data! :(\n- UnixTimeStampStart: %lld\n\n", UnixTimeStampStart.count());
            return;
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", LevelName},

            // Speedrun
            {"TimeAttackClock", speedrun->TdGameData->TimeAttackClock},
            {"TimeAttackDistance", speedrun->TdGameData->TimeAttackDistance},

            // World Info Time
            {"WorldTimeSeconds", pawn->WorldInfo->TimeSeconds},
            {"WorldRealTimeSeconds", pawn->WorldInfo->RealTimeSeconds},

            // Custom data
            {"AvgSpeed", (speedrun->TdGameData->TimeAttackDistance / speedrun->TdGameData->TimeAttackClock) * MS_TO_KPH},
            {"CheckpointInfo", VectorInfoToJson(SpeedrunCheckpointInfo)},
            {"ReactionTimeInfo", VectorInfoToJson(SpeedrunReactionTimeInfo)},
            {"PlayerDeathsInfo", VectorInfoToJson(SpeedrunPlayerDeathsInfo)}
        };

        SendJsonData(jsonData);
    }
}

static void OnTick(const float deltaTime)
{
    if (!Enabled || PlayerName.empty() || LevelName.empty() || LevelName == Map_MainMenu)
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
    Enabled = Settings::GetSetting("race", "enabled", false);
    SubmitRun = Settings::GetSetting("race", "submitRun", true);
    PlayerName = Settings::GetSetting("race", "playerName", "").get<std::string>();
    strncpy_s(NameInput, sizeof(NameInput) - 1, PlayerName.c_str(), sizeof(NameInput) - 1);

    Menu::AddTab("Leaderboard", LeaderboardTab);
    Engine::OnTick(OnTick);

    Engine::OnPreLevelLoad([](const wchar_t* levelNameW) {
        LevelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(levelNameW);

        std::transform(LevelName.begin(), LevelName.end(), LevelName.begin(), [](char c) {
            return tolower(c);
        });
    });

    return true;
}

std::string Leaderboard::GetName()
{
    return "Leaderboard";
}
