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
#include "../https/WinHttpClient.h"

// clang-format off

static auto enabled = true, timetrialMode = true, updatePointer = false;
static std::string levelName;
static json jsonData;

static Leaderboard::TimeTrial oldTimeTrial;
static Leaderboard::Speedrun oldSpeedrun;
static Leaderboard::Controller oldController;

// Timetrial
static Checkpoint checkpoints;
static float checkpointTopSpeed = 0.0f;
static float previousDistance = 0.0f;
static float previousTime = 0.0f;

// Speedrun
static int reactionTimeUses = 0;

// Both
static float overallTopSpeed = 0.0f;

static void SaveTimeTrialData(Classes::ATdSPTimeTrialGame* timetrial) 
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
    oldTimeTrial.CurrentTackData = timetrial->CurrentTackData;  
    oldTimeTrial.CurrentTimeData = timetrial->CurrentTimeData;  
    oldTimeTrial.TimeDataToBeat = timetrial->TimeDataToBeat;
    oldTimeTrial.RaceStartTimeStamp = timetrial->RaceStartTimeStamp;  
    oldTimeTrial.RaceEndTimeStamp = timetrial->RaceEndTimeStamp;
    oldTimeTrial.PlayerDistance = timetrial->PlayerDistance;
}

static void SaveSpeedRunData(Classes::ATdSPLevelRace* speedrun)
{
    oldSpeedrun.bRaceOver = speedrun->bRaceOver;
    oldSpeedrun.TimeAttackClock = speedrun->TdGameData->TimeAttackClock;
    oldSpeedrun.TimeAttackDistance = speedrun->TdGameData->TimeAttackDistance;
}

static void SaveControllerData(Classes::ATdPlayerController* controller)
{
    oldController.bReactionTime = controller->bReactionTime;
    oldController.ReactionTimeEnergy = controller->ReactionTimeEnergy;
}

static void SendPostData()
{
    static WinHttpClient client(L"secret url");

    // Set post data.
    std::string data = "?data=" + jsonData.dump();
    client.SetAdditionalDataToSend((BYTE *)data.c_str(), data.size());

    // Set request headers.
    wchar_t szSize[50] = L"";
    swprintf_s(szSize, L"%d", data.size());
    std::wstring headers = L"Content-Length: ";
    headers += szSize;
    headers += L"\r\nContent-Type: application/x-www-form-urlencoded\r\n";
    client.SetAdditionalRequestHeaders(headers);

    // Send HTTP post request.
    printf("post sending\n");
    client.SendHttpRequest(L"POST");
    printf("post sent\n");

    jsonData.clear();
}

static void LeaderboardTab() 
{ 
    if (ImGui::Checkbox("Enabled", &enabled)) {
        Settings::SetSetting("race", "enabled", enabled);
    }

    if (!enabled)
    {
        return;
    }

    if (ImGui::Checkbox("Time Trial", &timetrialMode)) {
        Settings::SetSetting("race", "timetrial", timetrialMode);
    }
}

// Reset values to their default values and clear vectors
static void ResetValues() 
{
    // Timetrial
    checkpoints.Clear();
    checkpointTopSpeed = 0.0f;
    previousTime = 0.0f;
    previousDistance = 0.0f;

    // Speedrun
    reactionTimeUses = 0;

    // Both
    overallTopSpeed = 0.0f;
}

static void TimeTrial(Classes::AWorldInfo* world, Classes::ATdPawn* pawn)
{
    const auto timetrial = Engine::GetTimeTrialGame(updatePointer);

    if (!timetrial) 
    {
        return;
    }

    if (updatePointer) 
    {
        printf("[TimeTrial] Player was found. \"updatePointer\" is set to false\n");
        updatePointer = false;
    }

    if (timetrial->RaceCountDownTimer == 1 && oldTimeTrial.RaceCountDownTimer == 2) 
    {
        timetrial->LastPlayerResetTime = 0.0f;
        timetrial->RaceStartTimeStamp = 0.0f;

        world->TimeSeconds = 0.0f;
        pawn->RollTriggerTime = 0.0f;

        ResetValues();
    }

    if (timetrial->CurrentTimeData.TotalTime == 0.0f && pawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) 
    {
        const float speed = sqrtf(powf(pawn->Velocity.X, 2) + powf(pawn->Velocity.Y, 2)) * 0.036f;

        if (speed > checkpointTopSpeed) 
        {
            checkpointTopSpeed = speed;
        }
            
        if (speed > overallTopSpeed) 
        {
            overallTopSpeed = speed;
        }
    }

    if (timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) 
    {
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

    if (timetrial->NumPassedCheckPoints == oldTimeTrial.NumCheckPoints && timetrial->NumPassedCheckPoints > oldTimeTrial.NumPassedCheckPoints) 
    {
        printf("============================================================================\n");
        printf("MapName: %s\n", levelName.c_str());
        printf("StretchId: %d\n", timetrial->ActiveTTStretch.GetValue());
        printf("----------------------------------------------------------------------------\n");
        printf("NumCheckPoints: %d\n", timetrial->NumCheckPoints);
        printf("NumPassedCheckPoints: %d\n", timetrial->NumPassedCheckPoints);
        printf("NumPassedTimerCheckPoints: %d\n", timetrial->NumPassedTimerCheckPoints);
        printf("----------------------------------------------------------------------------\n");
        printf("TotalTime: %f s\n", timetrial->CurrentTimeData.TotalTime);
        printf("Distance: %f m\n", timetrial->PlayerDistance);
        printf("AvgSpeed: %f km/h\n", (timetrial->PlayerDistance / timetrial->CurrentTimeData.TotalTime) * 3.6f);
        printf("TopSpeed: %f km/h\n", overallTopSpeed);
        printf("============================================================================\n");

        for (int i = 0; i < timetrial->NumCheckPoints; i++) 
        {
	        printf("Checkpoints[%d]->AvgSpeed: %f km/h\n", i, checkpoints.avgspeed[i]);	
	        printf("Checkpoints[%d]->TopSpeed: %f km/h\n", i, checkpoints.topspeed[i]);	
	        printf("Checkpoints[%d]->TimedCheckpoint: %s\n", i, checkpoints.timedCheckpoint[i] == true ? "true" : "false");	
	        printf("Checkpoints[%d]->IntermediateTimes: %f s\n", i, checkpoints.intermediateTimes[i]);	
	        printf("Checkpoints[%d]->AccumulatedIntermediateTimes: %f s\n", i, checkpoints.accumulatedIntermediateTimes[i]);
	        printf("Checkpoints[%d]->IntermediateDistances: %f m\n", i, checkpoints.intermediateDistances[i]);	
	        printf("Checkpoints[%d]->AccumulatedIntermediateDistances: %f m\n", i, checkpoints.accumulatedIntermediateDistances[i]);

            if (i != checkpoints.intermediateTimes.size() - 1) 
            {
                printf("----------------------------------------------------------------------------\n");	 
            }
	    }

        printf("============================================================================\n");

        if (!checkpoints.IsValid(timetrial->NumCheckPoints))
        {
            printf("Invalid Data!\n");
            return;
        }

        json jsonCheckpoints;

        for (int i = 0; i < timetrial->NumCheckPoints; i++) {
            json jsonData = {
                {"avgspeed", checkpoints.avgspeed[i]},
                {"topspeed", checkpoints.topspeed[i]},
                {"timedCheckpoint", checkpoints.timedCheckpoint[i] == true ? "true" : "false"},
                {"intermediateDistances", checkpoints.intermediateDistances[i]},
                {"accumulatedIntermediateDistances", checkpoints.accumulatedIntermediateDistances[i]},
                {"intermediateTimes", checkpoints.intermediateTimes[i]},
                {"accumulatedIntermediateTimes", checkpoints.accumulatedIntermediateTimes[i]}
            };

            jsonCheckpoints.push_back(jsonData);
        }

        jsonData = {
            {"MapName", levelName},
            {"StretchId", timetrial->ActiveTTStretch.GetValue()},
            {"TotalTime", timetrial->CurrentTimeData.TotalTime},
            {"Distance", timetrial->PlayerDistance},
            {"AvgSpeed", (timetrial->PlayerDistance / timetrial->CurrentTimeData.TotalTime) * 3.6f},
            {"TopSpeed", overallTopSpeed},
            {"CheckpointData", jsonCheckpoints},
        };

        // SendPostData();
        ResetValues();
    }

    SaveTimeTrialData(timetrial);
}

static void SpeedRun(Classes::AWorldInfo* world, Classes::ATdPawn* pawn, Classes::ATdPlayerController* controller)
{
    const auto speedrun = Engine::GetLevelRace(true);

    if (!speedrun) 
    {
        return;
    }
    
    if (speedrun->TdGameData->TimeAttackClock == 0.0f && world->TimeSeconds == 0.0f && speedrun->TdGameData->TimeAttackDistance != 0.0f) 
    {
        printf("[Speedrun] \"ResetValueslevel\" binding detected. Setting \"TimeAttackDistance\" to 0.0f\n");
        speedrun->TdGameData->TimeAttackDistance = 0.0f;
    }

    if (speedrun->TdGameData->TimeAttackClock == 0.0f && world->TimeSeconds == 0.0f && speedrun->TdGameData->TimeAttackDistance == 0.0f && checkpointTopSpeed != 0.0f) 
    {
        checkpointTopSpeed = 0;
        reactionTimeUses = 0;
    }

    if (!speedrun->bRaceOver) 
    {
        const float speed = sqrtf(powf(pawn->Velocity.X, 2) + powf(pawn->Velocity.Y, 2)) * 0.036f;

        if (speed > checkpointTopSpeed && pawn->MovementState != Classes::EMovement::MOVE_FallingUncontrolled) 
        {
            checkpointTopSpeed = speed;
        }
    }

    if (!speedrun->bRaceOver && controller->bReactionTime != oldController.bReactionTime && controller->bReactionTime) 
    {
        reactionTimeUses++;
        printf("[Speedrun] Reactiontime used\n");
    }

    if (speedrun->bRaceOver != oldSpeedrun.bRaceOver && world->TimeSeconds > 6.9f) 
    {
        printf("============================================================================\n");
        printf("TimeAttackClock: %.2f s\n", speedrun->TdGameData->TimeAttackClock);
        printf("TimeAttackDistance: %.2f m\n", speedrun->TdGameData->TimeAttackDistance);
        printf("World->TimeSeconds: %.2f s\n", world->TimeSeconds);
        printf("World->RealTimeSeconds: %.2f s\n", world->RealTimeSeconds);
        printf("----------------------------------------------------------------------------\n");
        printf("Average Speed: %.2f km/h\n", (speedrun->TdGameData->TimeAttackDistance / speedrun->TdGameData->TimeAttackClock) * 3.6f);
        printf("*Top Speed: %.2f km/h\n", checkpointTopSpeed);
        printf("*Reaction Time Used: %d\n", reactionTimeUses);
        printf("============================================================================\n");

        ResetValues();
    }

    SaveSpeedRunData(speedrun);
    SaveControllerData(controller);
}

static void OnTick(float deltaTime) {
    if (!enabled) 
    {
        return;
    }

    if (levelName.empty() || levelName == "tdmainmenu") 
    {
        if (timetrialMode && !updatePointer) 
        {
            printf("[TimeTrial] Player is in mainmenu or \"levelName\" is empty. \"updatePointer\" is set to true\n");
            updatePointer = true;
        }

        return;
    }

    const auto world = Engine::GetWorld();
    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();

    if (!pawn || !world || !controller) 
    {
        if (timetrialMode && !updatePointer) 
        {
            printf("[TimeTrial] Player isn't spawned. \"updatePointer\" is set to true\n");
            updatePointer = true;
        }

        return;
    }

    if (timetrialMode) 
    {
        TimeTrial(world, pawn);
    } 
    else 
    {
        SpeedRun(world, pawn, controller);
    }
}

bool Leaderboard::Initialize() 
{
    enabled = Settings::GetSetting("race", "enabled", false);
    timetrialMode = Settings::GetSetting("race", "timetrial", true);

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

std::string Leaderboard::GetName() { return "Leaderboard"; }