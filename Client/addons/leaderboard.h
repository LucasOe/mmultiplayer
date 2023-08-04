#pragma once

#include "../addon.h"

class Leaderboard : public Addon {
  public:

    typedef struct {
        int RaceFinishLineTime;                                     // 0x3AC (0x4)
        float RaceFinishLineTimer;                                  // 0x3B4 (0x4)
        int RaceCountDownTime;                                      // 0x3B8 (0x4)
        int RaceCountDownTimer;                                     // 0x3BC (0x4)
        int NumCheckPoints;                                         // 0x3D0 (0x4)
        int NumPassedCheckPoints;                                   // 0x3D4 (0x4)
        int NumPassedTimerCheckPoints;                              // 0x3D8 (0x4)
        float LastCheckpointTimeStamp;                              // 0x3DC (0x4)
        float LastPlayerResetTime;                                  // 0x3F4 (0x4)
        Classes::TEnumAsByte<Classes::ETTStretch> ActiveTTStretch;  // 0x3F9 (0x1)
        float QualifyingTime;                                       // 0x404 (0x4)
        float StarRatingTimes[0x3];                                 // 0x408 (0x4)
        Classes::FTrackData CurrentTackData;                        // 0x414 (0x10)
        Classes::FTimeData CurrentTimeData;                         // 0x424 (0x1C)
        Classes::FTimeData TimeDataToBeat;                          // 0x440 (0x1C)
        float RaceStartTimeStamp;                                   // 0x45C (0x4)
        float RaceEndTimeStamp;                                     // 0x460 (0x4)
        float PlayerDistance;                                       // 0x464 (0x4)
    } TimeTrial;

    typedef struct {
        unsigned long bRaceOver;                                    // 0x3C8 (0x4)

        // These can be found in TdGameData; 0x368 (0x4)
        float TimeAttackClock;                                      // 0x114 (0x4)
        float TimeAttackDistance;                                   // 0x118 (0x4)
    } Speedrun;

    typedef struct {
        int Health;                                                 // 0x2B8 (0x4)
        unsigned long bSRPauseTimer;                                // 0x41C (0x4)
    } Pawn;

    typedef struct {
        unsigned long bReactionTime;                                // 0x52C (0x4)
        float ReactionTimeEnergy;                                   // 0x5A8 (0x4)
    } Controller;

    bool Initialize();
    std::string GetName();
};

class Checkpoint {
  public:
    std::vector<float> avgspeed;
    std::vector<float> topspeed;
    std::vector<bool> timedCheckpoint;
    std::vector<float> intermediateDistances;
    std::vector<float> accumulatedIntermediateDistances;
    std::vector<float> intermediateTimes;
    std::vector<float> accumulatedIntermediateTimes;

    void Clear() {
        avgspeed.clear();
        topspeed.clear();
        timedCheckpoint.clear();
        intermediateDistances.clear();
        accumulatedIntermediateDistances.clear();
        intermediateTimes.clear();
        accumulatedIntermediateTimes.clear();
    }

    bool IsValid(int numPassedCheckpoints) {
        return numPassedCheckpoints == avgspeed.size() && 
            numPassedCheckpoints == topspeed.size() &&
            numPassedCheckpoints == timedCheckpoint.size() &&
            numPassedCheckpoints == intermediateDistances.size() &&
            numPassedCheckpoints == accumulatedIntermediateDistances.size() &&
            numPassedCheckpoints == intermediateTimes.size() &&
            numPassedCheckpoints == accumulatedIntermediateTimes.size();
    }
};