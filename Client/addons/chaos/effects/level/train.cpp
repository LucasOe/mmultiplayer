#pragma once

#include "../../effect.h"

enum class TrainEffect
{
    None,
    Slow,
    Fast,
    Strange
};

struct TrainDetail
{
    float NewPlayRate = 1.0f;
    float DefaultPlayRate = 1.5f;
    std::string SubLevelName;
    std::vector<Classes::USequenceObject*> SequenceObjects;
};

class Train : public Effect
{
private:
    bool NoTrainsFoundInSubLevel = false;
    float PlayerSpawnedTime = -1.0f;
    float TimeSinceLastChecked = 0.0f;
    TrainEffect TrainEffectType = TrainEffect::None;
    std::vector<TrainDetail> Trains = {};

public:
    Train(const std::string& name, TrainEffect trainEffect)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Long;
        LevelEffect = true;

        TrainEffectType = trainEffect; 
    }

    bool CanActivate() override
    {
        InitializeTrains();

        for (const auto& train : Trains)
        {
            if (!train.SequenceObjects.empty())
            {
                return true;
            }
        }

        return false;
    }

    void Initialize() override
    {
        NoTrainsFoundInSubLevel = false;
        PlayerSpawnedTime = 0.0f;
        TimeSinceLastChecked = 0.0f;

        const auto pawn = Engine::GetPlayerPawn();
        if (!pawn)
        {
            return;
        }

        PlayerSpawnedTime = pawn->SpawnTime; 
    }

    void Tick(const float deltaTime) override
    {
        const auto pawn = Engine::GetPlayerPawn();
        if (!pawn)
        {
            return;
        }

        TimeSinceLastChecked += deltaTime;

        // If player just spawned, modifiy the trains since they got reset
        if (PlayerSpawnedTime != pawn->SpawnTime)
        {
            PlayerSpawnedTime = pawn->SpawnTime;
            TimeSinceLastChecked = 0.0f;

            InitializeTrains();
            ModifyTrains(true);
        }

        // Don't check each tick, instead check every 1 second
        if (TimeSinceLastChecked >= 1.0f)
        {
            TimeSinceLastChecked = 0.0f;

            if (NoTrainsFoundInSubLevel)
            {
                InitializeTrains();
                NoTrainsFoundInSubLevel = false;
            }

            ModifyTrains(true);
        }
    }

    bool Shutdown() override
    {
        ModifyTrains(false);
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Level;
    }

    std::string GetClass() const override
    {
        return "Train";
    }

private:
    float RandomizePlayRate(float defaultPlayRate)
    {
        switch (TrainEffectType)
        {
            case TrainEffect::Slow:
                return RandomFloat(defaultPlayRate / 6.0f, defaultPlayRate / 2.0f);

            case TrainEffect::Fast:
                return RandomFloat(defaultPlayRate * 4.0f, defaultPlayRate * 6.0f);

            case TrainEffect::Strange:
                return RandomFloat(defaultPlayRate * -1.618f, defaultPlayRate * 3.145f);

            case TrainEffect::None:
            default:
                return defaultPlayRate;
        }
    }

    void FindTrain(const std::string& levelName, const ImVec2& pos, float defaultPlayRate)
    {
        TrainDetail train;
        train.NewPlayRate = RandomizePlayRate(defaultPlayRate);
        train.DefaultPlayRate = defaultPlayRate;
        train.SubLevelName = levelName;
        train.SequenceObjects = GetKismetSequenceObjects(levelName, pos);

        Trains.push_back(train);
    }

    void InitializeTrains()
    {
        if (LevelName.empty())
        {
            return;
        }

        Trains.clear();

        if (LevelName == "escape_p")
        {
            FindTrain("Escape_Plaza_Spt", ImVec2(-936, 3320), 1.25f);
            FindTrain("Escape_Plaza_Spt", ImVec2(-936, 4520), 1.25f);
        }
        else if (LevelName == "subway_p")
        {
            FindTrain("Subway_Plat_Spt", ImVec2(-464, 304), 1.5f);
            FindTrain("Subway_Tunnel_Spt", ImVec2(-464, 304), 1.5f);
        }
        else if (LevelName == "mall_p")
        {
            FindTrain("Mall_HW_Spt", ImVec2(-1832, -1048), 1.5f);
            FindTrain("Mall_HW_Spt", ImVec2(2096, -1096), 1.5f);
        }
        else if (LevelName == "factory_p")
        {
            FindTrain("Factory_Pursu_Spt", ImVec2(-464, 304), 0.8f);

            // This is a float variable which sets the matinee's playrate so we need to change this instead
            FindTrain("Factory_Pursu_Spt", ImVec2(-264, 792), 0.6f);
        }
    }

    void ModifyTrains(const bool applyNewPlayRate)
    {
        if (LevelName.empty())
        {
            return;
        }

        for (auto& train : Trains)
        {
            const bool subLevelLoaded = IsSubLevelLoaded(train.SubLevelName);
            const auto objectAmount = train.SequenceObjects.size();

            if ((subLevelLoaded && objectAmount == 0) || (!subLevelLoaded && objectAmount > 0))
            {
                NoTrainsFoundInSubLevel = true;
                return;
            }

            for (auto& sequenceObject : train.SequenceObjects)
            {
                if (TrainEffectType == TrainEffect::Strange)
                {
                    train.NewPlayRate = RandomizePlayRate(train.DefaultPlayRate);
                }

                const auto objName = sequenceObject->ObjName.IsValid() ? sequenceObject->ObjName.ToString() : "";

                if (objName == "Matinee")
                {
                    auto matinee = static_cast<Classes::USeqAct_Interp*>(sequenceObject);
                    if (!matinee) continue;

                    matinee->PlayRate = applyNewPlayRate ? train.NewPlayRate : train.DefaultPlayRate;
                }
                else if (objName == "Float")
                {
                    auto variable = static_cast<Classes::USeqVar_Float*>(sequenceObject);
                    if (!variable) continue;

                    variable->FloatValue = applyNewPlayRate ? train.NewPlayRate : train.DefaultPlayRate;
                }
            }
        }
    }
};

using SlowTrains = Train;
using FastTrains = Train;
using StrangeTrains = Train;

REGISTER_EFFECT(SlowTrains, "Slow Trains", TrainEffect::Slow);
REGISTER_EFFECT(FastTrains, "Fast Trains", TrainEffect::Fast);
REGISTER_EFFECT(StrangeTrains, "Strange Trains", TrainEffect::Strange);
