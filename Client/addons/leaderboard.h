#pragma once

#include <chrono>
#include "../addon.h"

class Leaderboard : public Addon 
{
  public:
    bool Initialize();
    std::string GetName();
};

typedef struct
{
    // ATdPlayerPawn
    Classes::FVector Location;
    int Health;
    unsigned long bSRPauseTimer;

    // ATdPlayerController
    unsigned long bReactionTime;
    float ReactionTimeEnergy;

    // ATdSPLevelRace
    unsigned long bRaceOver;
    float TimeAttackClock;
    float TimeAttackDistance;
    int ActiveCheckpointWeight;

    // ATdSPTimeTrialGame
    int RaceFinishLineTime;
    float RaceFinishLineTimer;
    int RaceCountDownTime;
    int RaceCountDownTimer;
    int NumCheckPoints;
    int NumPassedCheckPoints;
    int NumPassedTimerCheckPoints;
    float LastCheckpointTimeStamp;
    float LastPlayerResetTime;
    unsigned char bDelayPauseUntilRaceStarted;
    Classes::TEnumAsByte<Classes::ETTStretch> ActiveTTStretch;
    float QualifyingTime;
    float StarRatingTimes[0x3];
    Classes::FTrackData CurrentTackData;
    Classes::FTimeData CurrentTimeData;
    Classes::FTimeData TimeDataToBeat;
    float RaceStartTimeStamp;
    float RaceEndTimeStamp;
    float PlayerDistance;
} OldStruct;

typedef struct {
    float Time;
    float Distance;
    float AvgSpeed;
    float TopSpeed;
    float IntermediateTime;
    float IntermediateDistance;
    float WorldTimeSeconds;
    float WorldRealTimeSeconds;
    int CheckpointWeight;

    Classes::FVector Location;
    std::chrono::milliseconds UnixTime;
} InfoStruct;

typedef struct {
    float AvgSpeed;
    bool TimedCheckpoint;
    float IntermediateTime;
    float AccumulatedIntermediateTime;
    float IntermediateDistance;
    float AccumulatedIntermediateDistance;

    std::vector<InfoStruct> RespawnInfo;
    InfoStruct TopSpeedInfo;
    Classes::FVector Location;
    std::chrono::milliseconds UnixTime;
} TimeTrialInfoStruct;

