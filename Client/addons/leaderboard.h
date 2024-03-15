#pragma once

#include <chrono>
#include "../addon.h"

typedef struct
{
    // AWorldInfo
    float TimeSeconds;
    float RealTimeSeconds;

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

typedef struct
{
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
    std::chrono::seconds UnixTime;
} InfoStruct;

typedef struct
{
    float AvgSpeed;
    bool TimedCheckpoint;
    float IntermediateTime;
    float AccumulatedIntermediateTime;
    float IntermediateDistance;
    float AccumulatedIntermediateDistance;

    std::vector<InfoStruct> RespawnInfo;
    InfoStruct TopSpeedInfo;
    Classes::FVector Location;
    std::chrono::seconds UnixTime;
} TimeTrialInfoStruct;

typedef struct
{
    std::string RunId;
    std::string PlayerId;
    std::string DisplayName;
    int Rank;
    int SkillRating;
    float Time;
    float AvgSpeed;
    float Distance;
} LeaderboardDataStruct;

typedef struct
{
    std::string FixedId;
    std::string DisplayName;
    int SkillRating;
    std::string SessionToken;
} PlayerStruct;

static const char* GameModeTable[] = {
    "Time Trial",
    "Speedrun",
};

static const char* SortByTable[] = {
    "Top-Rankings",
    "Friends",
    "Center-On-Me",
};

static const char* TimeFrameTable[] = {
    "All-Time",
    "Monthly",
    "Weekly",
};

static const char* CourseTimeTrialTable[] = {
    "Playground One",
    "Playground Two",
    "Playground Three",
    "Edge",
    "Arland",
    "Flight",
    "Chase",
    "Stormdrains One",
    "Stormdrains Two",
    "Stormdrains Three",
    "Heat",
    "Burfield",
    "Cranes One",
    "Cranes Two",
    "New Eden",
    "Factory",
    "Office",
    "Convoy One",
    "Convoy Two",
    "Atrium One",
    "Atrium Two",
    "Shard One",
    "Shard Two",
};

static const char* CourseLevelRaceTable[] = {
    "Prologue",
    "Flight",
    "Jacknife",
    "Heat",
    "Ropeburn",
    "New Eden",
    "Pirandello Kruger",
    "The Boat",
    "Kate",
    "The Shard",
};

class Leaderboard : public Addon 
{
public:
    bool Initialize();
    std::string GetName();

    void HandleMsg(json &msg);
};
