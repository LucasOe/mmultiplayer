#include <codecvt>

#include "../engine.h"
#include "../imgui/imgui_mmultiplayer.h"

// Disable warnings that shows up when making a build with imgui_spinner
#pragma warning(push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4244)
#include "../imgui/imgui_spinner.h"
#pragma warning(pop)

#include "../menu.h"
#include "../util.h"
#include "../settings.h"

#include "client.h"
#include "leaderboard.h"
#include "../mmultiplayer.h"

// clang-format off

// Forward Decloration
class Leaderboard;

// Addon
static bool Enabled;

static bool IsMetric = true;
static bool PractiseMode;
static bool IsLoggedIn;
static bool IsLoggingIn;
static bool IsDownloading;

static char UsernameInput[0x20] = {0};
static char PasswordInput[0x80] = {0};
static std::string Username;
static std::string Password;

// Used for the leaderboard below the main leaderboard. Like the ingame, you can see what rank you have
static PlayerStruct PlayerInfo;
static LeaderboardDataStruct PlayerLeaderboardInfo;

static std::string LevelName;

// Used for comparing the previous value with the current value. Similar to ASL for Livesplit without the "Current"
// Example: timetrial->NumPassedCheckPoints > Old.NumPassedCheckPoints
static OldStruct Old;

// Info about the time, avgspeed, and etc during the current tick
static InfoStruct Info;
static TimeTrialInfoStruct TimeTrialInfo;

// Holds the leaderboard data for the current selected leaderboard
static std::vector<LeaderboardDataStruct> LeaderboardData;
static int GameModeValue;
static int CourseTimeTrialValue;
static int CourseLevelRaceValue;
static int SortByValue;
static int TimeFrameValue;

// Extra info collected during a Time Trial run
static std::vector<InfoStruct> TimeTrialCheckpointRespawnInfo;
static std::vector<TimeTrialInfoStruct> TimeTrialCheckpointInfo;

// Extra info collected during a Chapter Speedrun run
static std::vector<InfoStruct> SpeedrunCheckpointInfo;
static std::vector<InfoStruct> SpeedrunReactionTimeInfo;
static std::vector<InfoStruct> SpeedrunPlayerDeathsInfo;

// Timestamps in unix time. These are used to know if the run was started properly or not
static std::chrono::seconds UnixTimeStampStart = std::chrono::seconds(0);
static std::chrono::seconds UnixTimeStampEnd = std::chrono::seconds(0);

// Topspeed info used by both time trial and chapter speedrun
static float TopSpeed = 0.0f;
static InfoStruct TopSpeedInfo = {0};

static float PreviousTime = 0.0f;
static float PreviousDistance = 0.0f;

static void SaveData(const Classes::ATdPlayerPawn* pawn, const Classes::ATdPlayerController* controller, const Classes::ATdSPTimeTrialGame* timetrial, const Classes::ATdSPLevelRace* speedrun, const Classes::AWorldInfo* world)
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

    if (world)
    {
        Old.TimeSeconds = world->TimeSeconds;
        Old.RealTimeSeconds = world->RealTimeSeconds;
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

    UnixTimeStampStart = std::chrono::seconds(0);
    UnixTimeStampEnd = std::chrono::seconds(0);

    TopSpeed = 0.0f;
    TopSpeedInfo = {0};

    PreviousTime = 0.0f;
    PreviousDistance = 0.0f;
}

static void ShowLoadingText(const char* text, const float radius = 7.0f, const float thickness = 3.0f, const float speed = 10.0f)
{
    ImSpinner::SpinnerAng("", radius, thickness, ImSpinner::white, ImSpinner::half_white, speed, ImSpinner::PI_DIV_2);
    ImGui::SameLine(32.0f);

    // TODO: Adjust text so it aligns with the spinning icon
    ImGui::Text(text);
}

static void LeaderboardTab()
{
    /*
    if (ImGui::Begin("Leaderboard Debug##leaderboard-debug"))
    {
        ImGui::Text("IsMetric: %s", IsMetric ? "true" : "false");
        ImGui::Text("PractiseMode: %s", PractiseMode ? "true" : "false");
        ImGui::Text("IsLoggedIn: %s", IsLoggedIn ? "true" : "false");
        ImGui::Text("IsLoggingIn: %s", IsLoggingIn ? "true" : "false");
        ImGui::Text("IsDownloading: %s", IsDownloading ? "true" : "false");

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::Text("PlayerInfo.FixedId: %s", PlayerInfo.FixedId.c_str());
        ImGui::Text("PlayerInfo.SkillRating: %d", PlayerInfo.SkillRating);
        ImGui::Text("PlayerInfo.DisplayName: %s", PlayerInfo.DisplayName.c_str());
        ImGui::Text("PlayerInfo.SessionToken: %s", PlayerInfo.SessionToken.c_str());

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::Text("PlayerLeaderboardInfo.RunId: %s", PlayerLeaderboardInfo.RunId.c_str());
        ImGui::Text("PlayerLeaderboardInfo.Rank: %d", PlayerLeaderboardInfo.Rank);
        ImGui::Text("PlayerLeaderboardInfo.PlayerId: %s", PlayerLeaderboardInfo.PlayerId.c_str());
        ImGui::Text("PlayerLeaderboardInfo.DisplayName: %s", PlayerLeaderboardInfo.DisplayName.c_str());
        ImGui::Text("PlayerLeaderboardInfo.SkillRating: %d", PlayerLeaderboardInfo.SkillRating);
        ImGui::Text("PlayerLeaderboardInfo.Time: %f", PlayerLeaderboardInfo.Time);
        ImGui::Text("PlayerLeaderboardInfo.AvgSpeed: %f", PlayerLeaderboardInfo.AvgSpeed);
        ImGui::Text("PlayerLeaderboardInfo.Distance: %f", PlayerLeaderboardInfo.Distance);

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::Text("GameModeValue: %d", GameModeValue);
        ImGui::Text("CourseTimeTrialValue: %d", CourseTimeTrialValue);
        ImGui::Text("CourseLevelRaceValue: %d", CourseLevelRaceValue);
        ImGui::Text("SortByValue: %d", SortByValue);
        ImGui::Text("TimeFrameValue: %d", TimeFrameValue);

        ImGui::End();
    }
    */

    if (ImGui::Checkbox("Enabled##leaderboard-enabled", &Enabled)) {
        Settings::SetSetting("leaderboard", "enabled", Enabled);
    }
    
    if (!Enabled) {
        return;
    }

    if (ImGui::Checkbox("Metric##leaderboard-metric", &IsMetric)) {
        Settings::SetSetting("leaderboard", "metric", IsMetric);
    }

    //ImGui::SameLine();
    //ImGui::HelpMarker("Display 20.28 km/h or 12.60 mph");
    ImGui::SpacingY();

    Client client;
    if (!client.IsConnected())
    {
        ImGui::Text("No connection to the server or mmultiplayer is disabled\nIn the Multiplayer tab, right under it, it should say \"Status: Connected\"");
        return;
    }

    if (!IsLoggedIn && !IsLoggingIn)
    {
        ImGui::Text("Username");
        if (ImGui::InputText("##leaderboard-username", UsernameInput, sizeof(UsernameInput)))
        {
            Username = UsernameInput;
        }

        ImGui::Text("Password");
        if (ImGui::InputText("##leaderboard-password", PasswordInput, sizeof(PasswordInput), ImGuiInputTextFlags_Password))
        {
            Password = PasswordInput;
        }

        ImGui::Dummy(ImVec2(0.0f, 2.0f));

        ImGui::BeginDisabled(Username.size() < 3 || Password.size() < 3);
        if (ImGui::Button("Login##leaderboard-login"))
        {
            json json = {
                {"type", "leaderboard"},
                {"subtype", "login"},
                {"username", Username},
                {"password", Password}
            };

            IsLoggingIn = client.SendJsonMessage(json);
        }
        ImGui::EndDisabled();
    }
    else if (IsLoggingIn)
    {
        ShowLoadingText("Logging in");
    }
    else
    {
        ImGui::Text("Logged in as %s", PlayerInfo.DisplayName.c_str());
        ImGui::Checkbox("Practise", &PractiseMode);
        ImGui::SameLine();
        ImGui::HelpMarker("The runs you do wont be uploaded to the server if set to false"); 
        
        if (ImGui::Button("Logout##leaderboard-logout"))
        {
            IsLoggedIn = false;
        }
    }

    ImGui::SpacingY();

    ImGui::BeginDisabled(IsDownloading || IsLoggingIn);
    if (ImGui::Button("Update##leaderboard-download-runs"))
    {
        LeaderboardData.clear();
        
        json json = {
            {"type", "leaderboard"},
            {"subtype", "get"},
            {"gamemode", std::to_string(GameModeValue)},
            {"course", std::to_string(GameModeValue == TIMETRIAL ? CourseTimeTrialValue : CourseLevelRaceValue)},
            {"sortby", std::to_string(SortByValue)},
            {"timeframe", std::to_string(TimeFrameValue)}
        };

        if (IsLoggedIn && SortByValue != 0 /* 0 == All-Top */)
        {
            json.push_back({ "player", PlayerInfo.FixedId });
        }

        IsDownloading = client.SendJsonMessage(json);

    }
    ImGui::EndDisabled();

    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    if (ImGui::CollapsingHeader("Leaderboard"))
    {
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        static float offsetFromStart = 128.0f;

        ImGui::Text("Game Mode:");
        ImGui::SameLine(offsetFromStart);
        ImGui::Combo("##leaderboard-filter-gamemode", &GameModeValue, GameModeTable, IM_ARRAYSIZE(GameModeTable));

        ImGui::Text("Select Course:");
        ImGui::SameLine(offsetFromStart);
        if (GameModeValue == TIMETRIAL)
        {
            ImGui::Combo("##leaderboard-filter-course", &CourseTimeTrialValue, CourseTimeTrialTable, IM_ARRAYSIZE(CourseTimeTrialTable));
        }
        else
        {
            ImGui::Combo("##leaderboard-filter-course", &CourseLevelRaceValue, CourseLevelRaceTable, IM_ARRAYSIZE(CourseLevelRaceTable));
        }

        ImGui::Text("Sort By:");
        ImGui::SameLine(offsetFromStart);
        ImGui::Combo("##leaderboard-filter-sortby", &SortByValue, SortByTable, IM_ARRAYSIZE(SortByTable));

        ImGui::Text("Time Frame:");
        ImGui::SameLine(offsetFromStart);
        ImGui::Combo("##leaderboard-filter-timeframe", &TimeFrameValue, TimeFrameTable, IM_ARRAYSIZE(TimeFrameTable));
        ImGui::Dummy(ImVec2(0.0f, 12.0f));

        static ImGuiTableFlags tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner;
        static ImGuiTableFlags columnFlags = ImGuiTableColumnFlags_WidthFixed;

        if (IsDownloading)
        {
            char buffer[0x69];
            sprintf_s(buffer, "Downloading \"%s\" Runs", GameModeValue == TIMETRIAL ? CourseTimeTrialTable[CourseTimeTrialValue] : CourseLevelRaceTable[CourseLevelRaceValue]);
            
            ShowLoadingText(buffer);
        }
        else
        {
            static int columns = 6;
            static int rowsToShow = 10;

            static float rankWidth = 40.0f;
            static float playerWidth = 256.0f;
            static float ratingWidth = 48.0f;
            static float timeWidth = 92.0f;
            static float avgSpeedWidth = 92.0f;
            static float distanceWidth = 92.0f;

            static float rowHeight = 28.8f;
            static float tableHeight = rowsToShow * rowHeight;

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5.0f, 5.0f));
            if (ImGui::BeginTable("leaderboard-table-main", columns, tableFlags, ImVec2(0.0f, tableHeight)))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Rank", columnFlags, rankWidth);
                ImGui::TableSetupColumn("Player", columnFlags, playerWidth);
                ImGui::TableSetupColumn("Rating", columnFlags, ratingWidth);
                ImGui::TableSetupColumn("Time", columnFlags, timeWidth);
                ImGui::TableSetupColumn("Avg Speed", columnFlags, avgSpeedWidth);
                ImGui::TableSetupColumn("Distance", columnFlags, distanceWidth);
                ImGui::TableHeadersRow();

                for (size_t row = 0; row < LeaderboardData.size(); row++)
                {
                    ImGui::TableNextRow();
                    
                    ImGui::TableNextColumn(); 
                    ImGui::Text("%d", LeaderboardData[row].Rank);

                    ImGui::TableNextColumn(); 
                    ImGui::Text("%s", LeaderboardData[row].DisplayName.c_str());

                    ImGui::TableNextColumn(); 
                    ImGui::Text("%d", LeaderboardData[row].SkillRating);

                    ImGui::TableNextColumn(); 
                    ImGui::Text(FormatTime(LeaderboardData[row].Time).c_str());

                    ImGui::TableNextColumn(); 
                    ImGui::Text(FormatAverageSpeed(LeaderboardData[row].AvgSpeed, IsMetric).c_str());

                    ImGui::TableNextColumn(); 
                    ImGui::Text(FormatDistance(LeaderboardData[row].Distance, IsMetric).c_str());
                }

                ImGui::EndTable();
            }

            if (!IsLoggedIn)
            {
                ImGui::PopStyleVar();
                ImGui::TreePop();
                return;
            }

            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            if (ImGui::BeginTable("leaderboard-table-solo", columns, tableFlags, ImVec2(0.0f, rowHeight)))
            {
                ImGui::TableSetupColumn("Rank", columnFlags, rankWidth);
                ImGui::TableSetupColumn("Player", columnFlags, playerWidth);
                ImGui::TableSetupColumn("Rating", columnFlags, ratingWidth);
                ImGui::TableSetupColumn("Time", columnFlags, timeWidth);
                ImGui::TableSetupColumn("Avg Speed", columnFlags, avgSpeedWidth);
                ImGui::TableSetupColumn("Distance", columnFlags, distanceWidth);

                ImGui::TableNextColumn();
                if (PlayerLeaderboardInfo.Rank == 0)
                {
                    ImGui::Text("-");
                }
                else
                {
                    ImGui::Text("%d", PlayerLeaderboardInfo.Rank);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", PlayerInfo.DisplayName.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%d", PlayerInfo.SkillRating);

                ImGui::TableNextColumn();
                ImGui::Text(FormatTime(PlayerLeaderboardInfo.Time).c_str());

                ImGui::TableNextColumn();
                ImGui::Text(FormatAverageSpeed(PlayerLeaderboardInfo.AvgSpeed, IsMetric).c_str());

                ImGui::TableNextColumn();
                ImGui::Text(FormatDistance(PlayerLeaderboardInfo.Distance, IsMetric).c_str());

                ImGui::EndTable();
            }

            ImGui::PopStyleVar();
            ImGui::TreePop();
        }
    }
}

static void SendJsonPostData(json jsonData)
{
    jsonData.push_back({"FixedId", PlayerInfo.FixedId});
    jsonData.push_back({"SessionToken", PlayerInfo.SessionToken});
    jsonData.push_back({"UnixTimeStampStart", UnixTimeStampStart.count()});
    jsonData.push_back({"UnixTimeStampEnd", UnixTimeStampEnd.count()});

    json json = {
        {"type", "leaderboard"},
        {"subtype", "post"},
        {"body", jsonData.dump()}
    };

    Client client;
    if (client.IsConnected())
    {
        if (client.SendJsonMessage(json))
        {
            printf("data sent successfully! c:\n\n");
        }
        else
        {
            printf("data failed to send! :c\n\n");
        }
    }

    ResetData();
}

static json InfoStructToJson(const InfoStruct info)
{
    if (info.WorldRealTimeSeconds == 0.0f)
    {
        return { 
            {"Time", info.Time},
            {"Distance", info.Distance},
            {"AvgSpeed", info.AvgSpeed},
            {"TopSpeed", info.TopSpeed},
            {"IntermediateTime", info.IntermediateTime},
            {"IntermediateDistance", info.IntermediateDistance},
            {"PlayerLocation", {
                info.Location.X,
                info.Location.Y,
                info.Location.Z
            }},
            {"UnixTime", info.UnixTime.count()},
        };
    }

    return { 
        {"Time", info.Time},
        {"Distance", info.Distance},
        {"AvgSpeed", info.AvgSpeed},
        {"TopSpeed", info.TopSpeed},
        {"WorldTimeSeconds", info.WorldTimeSeconds},
        {"WorldRealTimeSeconds", info.WorldRealTimeSeconds},
        {"CheckpointWeight", info.CheckpointWeight},
        {"PlayerLocation", {
            info.Location.X,
            info.Location.Y,
            info.Location.Z
        }},
        {"UnixTime", info.UnixTime.count()},
    };
}

static json VectorInfoStructToJson(const std::vector<InfoStruct> info)
{
    json json;

    for (size_t i = 0; i < info.size(); i++)
    {
        json.push_back(InfoStructToJson(info[i]));
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
    Info.UnixTime = GetCurrentTimeInSeconds();

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

        if (TimeTrialCheckpointInfo.size() != timetrial->NumPassedCheckPoints || UnixTimeStampStart == std::chrono::seconds(0))
        {
            printf("timetrial: invalid data! :(\n");
            return;
        }

        json jsonIntermediateDistances;
        json jsonCheckpoints;
        json jsonCheckpointData;

        for (int i = 0; i < timetrial->NumCheckPoints; i++)
        {
            json jsonRespawns;

            jsonCheckpoints.push_back({
                {"AvgSpeed", TimeTrialCheckpointInfo[i].AvgSpeed},
                {"TimedCheckpoint", TimeTrialCheckpointInfo[i].TimedCheckpoint},
                {"IntermediateTime", TimeTrialCheckpointInfo[i].IntermediateTime},
                {"IntermediateDistance", TimeTrialCheckpointInfo[i].IntermediateDistance},
                {"UnixTime", TimeTrialCheckpointInfo[i].UnixTime.count()},
                {"RespawnInfo", VectorInfoStructToJson(TimeTrialCheckpointInfo[i].RespawnInfo)},
                {"TopSpeedInfo", InfoStructToJson(TimeTrialCheckpointInfo[i].TopSpeedInfo)},
            });
        }

        const auto start = static_cast<Classes::ATdTimeTrialStart*>(timetrial->StartPoints[0]);

        jsonCheckpointData.push_back({
            // AActor
            {"Location", {
                start->Location.X,
                start->Location.Y,
                start->Location.Z
            }},
            {"Rotation", {
                ConvertRotationToFloat(start->Rotation.Pitch),
                ConvertRotationToFloat(start->Rotation.Roll),
                ConvertRotationToFloat(start->Rotation.Yaw)
            }},

            // ATdTimeTrialStart
            {"TrackIndex", start->TrackIndex.GetValue()},
        });

        for (int i = 0; i < timetrial->NumPassedCheckPoints; i++)
        {
            const auto track = static_cast<Classes::ATdTimerCheckpoint*>(timetrial->CheckpointManager->ActiveTrack[i]);

            json jsonBelongToTracks;

            for (size_t j = 0; j < track->BelongToTracks.Num(); j++)
            {
                jsonBelongToTracks.push_back({
                    {"TrackIndex", track->BelongToTracks[j].TrackIndex.GetValue()},
                    {"OrderIndex", track->BelongToTracks[j].OrderIndex},
                    {"bNoIntermediateTime", track->BelongToTracks[j].bNoIntermediateTime == TRUE ? true : false}
                });
            }

            jsonCheckpointData.push_back({
                // AActor
                {"Location", {
                    track->Location.X, 
                    track->Location.Y, 
                    track->Location.Z
                }},

                // ATdTimerCheckpoint
                {"BelongToTracks", jsonBelongToTracks},
                {"CustomHeight", track->CustomHeight},
                {"CustomWidthScale", track->CustomWidthScale},
                {"bNoRespawn", track->bNoRespawn == TRUE ? true : false},
                {"InitialHeight", track->InitialHeight},
                {"InitialRadius", track->InitialRadius},

                // ATdPlaceableCheckpoint
                {"bShouldBeBased", track->bShouldBeBased == TRUE ? true : false},
            });
        }

        for (int i = 0; i < timetrial->NumPassedTimerCheckPoints; i++)
        {
            jsonIntermediateDistances.push_back(timetrial->CurrentTackData.IntermediateDistance[i]);
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", LevelName},
            {"Gamemode", "TimeTrial"},

            // Time Trial data
            {"Time", timetrial->CurrentTimeData.TotalTime},
            {"Distance", timetrial->PlayerDistance},
            {"AvgSpeed", (timetrial->PlayerDistance / timetrial->CurrentTimeData.TotalTime) * MS_TO_KPH},

            // Custom data
            {"CheckpointData", jsonCheckpoints},

            // These are sent in order to verify that the run is valid
            {"TrackData", {
                {"ActiveTTStretch", timetrial->ActiveTTStretch.GetValue()},
                {"QualifyingTime", timetrial->QualifyingTime},
                {"StarRatingTimes", {
                    timetrial->StarRatingTimes[0],
                    timetrial->StarRatingTimes[1],
                    timetrial->StarRatingTimes[2]
                }},
                {"TotalDistance", timetrial->CurrentTackData.TotalDistance},
                {"IntermediateDistances", jsonIntermediateDistances},
                {"CheckpointData", jsonCheckpointData}
            }}
        };

        SendJsonPostData(jsonData);
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
    Info.UnixTime = GetCurrentTimeInSeconds();

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
        UnixTimeStampStart = GetCurrentTimeInSeconds();
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
        if (UnixTimeStampStart == std::chrono::seconds(0))
        {
            printf("speedrun: invalid data! :(\n");
            return;
        }

        json jsonLevelStreaming;

        auto levels = speedrun->WorldInfo->StreamingLevels;
        for (size_t i = 0; i < levels.Num(); i++)
        {
            auto level = levels.GetByIndex(i);
            if (level)
            {
                jsonLevelStreaming.push_back(level->PackageName.GetName());
            }
        }

        json jsonData = {
            // The current map name in lower case
            {"MapName", LevelName},
            {"Gamemode", "LevelRace"},

            // ATdSPLevelRace
            {"Time", speedrun->TdGameData->TimeAttackClock},
            {"Distance", speedrun->TdGameData->TimeAttackDistance},
            {"AvgSpeed", (speedrun->TdGameData->TimeAttackDistance / speedrun->TdGameData->TimeAttackClock) * MS_TO_KPH},

            // AWorldInfo
            {"TimeSeconds", pawn->WorldInfo->TimeSeconds},
            {"RealTimeSeconds", pawn->WorldInfo->RealTimeSeconds},

            // Custom data
            {"LevelStreamingData", jsonLevelStreaming},
            {"CheckpointInfo", VectorInfoStructToJson(SpeedrunCheckpointInfo)},
            {"ReactionTimeInfo", VectorInfoStructToJson(SpeedrunReactionTimeInfo)},
            {"PlayerDeathsInfo", VectorInfoStructToJson(SpeedrunPlayerDeathsInfo)}
        };

        SendJsonPostData(jsonData);
    }
}

static void OnTick(const float deltaTime)
{
    if (!Enabled || LevelName.empty() || LevelName == Map_MainMenu || PractiseMode)
    {
        return;
    }

    auto world = Engine::GetWorld();
    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (!world || !pawn || !controller)
    {
        return;
    }

    if (world)
    {
        std::string game = world->Game->GetName();

        if (game.find("TdSPTimeTrialGame") != std::string::npos)
        {
            auto timetrial = static_cast<Classes::ATdSPTimeTrialGame*>(world->Game);

            OnTickTimeTrial(timetrial);
            SaveData(pawn, controller, timetrial, nullptr, world);
        }
        else if (game.find("TdSPLevelRace") != std::string::npos)
        {
            auto speedrun = static_cast<Classes::ATdSPLevelRace*>(world->Game);

            OnTickSpeedRun(speedrun);
            SaveData(pawn, controller, nullptr, speedrun, world);
        }
    }
}

void Leaderboard::HandleMsg(json &msg)
{
    const auto msgSubType = msg["subtype"];
    const auto msgData = msg["data"];
    const auto msgStatusCode = msg["statusCode"];

    printf("data: %s", msgData.get<std::string>().c_str());

    auto json = json::parse(msgData.get<std::string>());

    if (msgStatusCode != 200)
    {
        // TODO: Handle status codes that isn't 200
        return;
    }

    if (msgSubType == "login")
    {
        PlayerInfo.FixedId = json["FixedId"].get<std::string>();
        PlayerInfo.DisplayName = json["DisplayName"].get<std::string>();
        PlayerInfo.SkillRating = json["SkillRating"];
        PlayerInfo.SessionToken = json["SessionToken"].get<std::string>();

        IsLoggingIn = false;
        IsLoggedIn = true;
    }
    else if (msgSubType == "player")
    {
        IsDownloading = false;

        if (msgData.get<std::string>() == "no")
        {
            // Player doesn't have a run on the seleted filters
            return;
        }

        int i = 0;
        PlayerLeaderboardInfo.RunId = json[i++].get<std::string>();
        PlayerLeaderboardInfo.Rank = json[i++];
        PlayerLeaderboardInfo.PlayerId = json[i++].get<std::string>();
        PlayerLeaderboardInfo.DisplayName = PlayerInfo.DisplayName = json[i++].get<std::string>();
        PlayerLeaderboardInfo.Time = json[i++];
        PlayerLeaderboardInfo.Distance = json[i++];
        PlayerLeaderboardInfo.AvgSpeed = json[i++];
        PlayerLeaderboardInfo.SkillRating = PlayerInfo.SkillRating = json[i++];
    }
    else if (msgSubType == "runs")
    {
        // TODO: In go, make it so it sends like 5 or 10 runs at the time, then only clear the leaderboard if it's runs from 0 to x
        // TODO: Show the leaderboard once we have downloaded all the runs, maybe update the spinner text with a percentage sign?

        for (size_t i = 0; i < json.size(); i++)
        {
            LeaderboardDataStruct run;
            int j = 0;

            run.RunId = json[i][j++].get<std::string>();
            run.Rank = json[i][j++];
            run.PlayerId = json[i][j++].get<std::string>();
            run.DisplayName = json[i][j++].get<std::string>();
            run.Time = json[i][j++];
            run.Distance = json[i][j++];
            run.AvgSpeed = json[i][j++];
            run.SkillRating = json[i][j++];

            LeaderboardData.push_back(run);

            if (IsLoggedIn && run.PlayerId == PlayerInfo.FixedId)
            {
                PlayerLeaderboardInfo = run;
                PlayerInfo.DisplayName = run.DisplayName;
                PlayerInfo.SkillRating = run.SkillRating;
            }
        }

        IsDownloading = false;

        /*
        int batch = 0;
        if (batch == 10)
        {
            // If were not in the leaderboard after download is complete, send a get request to get the rank we're on
            if (PlayerLeaderboardInfo.Rank == 0)
            {
                Client client;
                client.SendJsonMessage({
                    {"type", "leaderboard"},
                    {"subtype", "get"},
                    {"gamemode", std::to_string(GameModeValue)},
                    {"course", std::to_string(GameModeValue == TIMETRIAL ? CourseTimeTrialValue : CourseLevelRaceValue)},
                    {"sortby", std::to_string(SortByValue)},
                    {"timeframe", std::to_string(TimeFrameValue)},
                    {"player", PlayerInfo.FixedId},
                });
            }
            else
            {
                IsDownloading = false;
            }
        }
        */
    }
}

bool Leaderboard::Initialize()
{
    Menu::AddTab("Leaderboard", LeaderboardTab);
    Engine::OnTick(OnTick);

    Engine::OnPreLevelLoad([](const wchar_t* levelNameW) 
    {
        LevelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(levelNameW);

        std::transform(LevelName.begin(), LevelName.end(), LevelName.begin(), [](char c) 
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
