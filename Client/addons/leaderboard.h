#pragma once

#include <chrono>
#include "../addon.h"

class Leaderboard : public Addon {
  public:
    class Old {
      public:
        typedef struct {
            int RaceFinishLineTime;
            float RaceFinishLineTimer;
            int RaceCountDownTime;
            int RaceCountDownTimer;
            int NumCheckPoints;
            int NumPassedCheckPoints;
            int NumPassedTimerCheckPoints;
            float LastCheckpointTimeStamp;
            float LastPlayerResetTime;
            Classes::TEnumAsByte<Classes::ETTStretch> ActiveTTStretch;
            float QualifyingTime;
            float StarRatingTimes[0x3];
            Classes::FTrackData CurrentTackData;
            Classes::FTimeData CurrentTimeData;
            Classes::FTimeData TimeDataToBeat;
            float RaceStartTimeStamp;
            float RaceEndTimeStamp;
            float PlayerDistance;
        } TimeTrial;

        typedef struct {
            unsigned long bRaceOver;
            float TimeAttackClock;
            float TimeAttackDistance;
            int ActiveCheckpointWeight;
        } Speedrun;

        typedef struct {
            int Health;
            unsigned long bSRPauseTimer;
        } Pawn;

        typedef struct {
            unsigned long bReactionTime;
            float ReactionTimeEnergy;
        } Controller;
    };

    bool Initialize();
    std::string GetName();
};

typedef struct {
    float worldTimeSeconds;
    float worldRealTimeSeconds;
    float time;
    float avgspeed;
    float topspeed;
    float distance;
    float intermediateDistance;
    float intermediateTime;
    int checkpointWeight;
    std::chrono::milliseconds unixTime;
} Info;

typedef struct {
    float avgspeed;
    Info topSpeedInfo;
    bool timedCheckpoint;
    float intermediateDistance;
    float accumulatedIntermediateDistance;
    float intermediateTime;
    float accumulatedIntermediateTime;
    std::vector<Info> respawnInfo;
    std::chrono::milliseconds checkpointTouchedUnixTime;
} TimeTrialInfo;
