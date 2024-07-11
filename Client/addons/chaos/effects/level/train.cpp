#pragma once

#include "../../effect.h"
#include <unordered_map>

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
    bool SubLevelUnloaded = false;
    float PlayerSpawnedTime = -1.0f;
    float TimeSinceLastChecked = 0.0f;
    TrainEffect TrainEffectType = TrainEffect::None;
    std::unordered_map<std::string, std::vector<TrainDetail>> TrainDetailsMap;

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
        const auto& trains = GetTrains();

        for (const auto& train : trains)
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
        SubLevelUnloaded = false;
        TimeSinceLastChecked = 0.0f;

        const auto pawn = Engine::GetPlayerPawn();
        
        if (!pawn)
        {
            PlayerSpawnedTime = 0.0f;
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

            if (SubLevelUnloaded)
            {
                InitializeTrains();
                SubLevelUnloaded = false;
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
        if (TrainEffectType == TrainEffect::Strange)
        {
            return RandomFloat(defaultPlayRate * -1.618f, defaultPlayRate * 3.145f);
        }

        switch (TrainEffectType)
        {
            case TrainEffect::Slow:
                return RandomFloat(defaultPlayRate / 6.0f, defaultPlayRate / 2.0f);

            case TrainEffect::Fast:
                return RandomFloat(defaultPlayRate * 4.0f, defaultPlayRate * 6.0f);

            case TrainEffect::None:
            default:
                return defaultPlayRate;
        }
    }

    void FindTrain(std::vector<TrainDetail>& trains, const std::string& levelName, const ImVec2& pos, float defaultPlayRate)
    {
        TrainDetail train;
        train.NewPlayRate = RandomizePlayRate(defaultPlayRate);
        train.DefaultPlayRate = defaultPlayRate;
        train.SubLevelName = levelName;
        train.SequenceObjects = GetKismetSequenceObjects(levelName, pos);

        trains.push_back(train);
    }

    const std::vector<TrainDetail>& GetTrains()
    {
        const auto levelName = GetLevelName();
        if (levelName.empty())
        {
            return std::vector<TrainDetail>();
        }

        return TrainDetailsMap[levelName];
    }

    void InitializeTrains()
    {
        const auto levelName = GetLevelName();
        if (levelName.empty())
        {
            return;
        }

        TrainDetailsMap.clear();
        auto& trains = TrainDetailsMap[levelName];

        if (levelName == "escape_p")
        {
            FindTrain(trains, "Escape_Plaza_Spt", ImVec2(-936, 3320), 1.25f);
            FindTrain(trains, "Escape_Plaza_Spt", ImVec2(-936, 4520), 1.25f);
        }
        else if (levelName == "subway_p")
        {
            FindTrain(trains, "Subway_Plat_Spt", ImVec2(-464, 304), 1.5f);
            FindTrain(trains, "Subway_Tunnel_Spt", ImVec2(-464, 304), 1.5f);
        }
        else if (levelName == "mall_p")
        {
            FindTrain(trains, "Mall_HW_Spt", ImVec2(-1832, -1048), 1.5f);
            FindTrain(trains, "Mall_HW_Spt", ImVec2(2096, -1096), 1.5f);
        }
        else if (levelName == "factory_p")
        {
            FindTrain(trains, "Factory_Pursu_Spt", ImVec2(-464, 304), 0.8f);

            // This is a float variable which sets the matinee's playrate so we need to change this instead
            FindTrain(trains, "Factory_Pursu_Spt", ImVec2(-264, 792), 0.6f);
        }
    }

    bool IsSubLevelLoaded(const std::string& levelName)
    {
        const auto world = Engine::GetWorld();
        if (!world)
        {
            return false;
        }

        for (size_t i = 0; i < world->StreamingLevels.Num(); i++)
        {
            const auto level = world->StreamingLevels[i];
            if (!level || !level->LoadedLevel)
            {
                continue;
            }

            if (level->PackageName.GetName() == levelName)
            {
                return true;
            }
        }

        return false;
    }

    void ModifyTrains(const bool applyNewPlayRate)
    {
        const auto levelName = GetLevelName();
        if (levelName.empty())
        {
            return;
        }

        auto& trains = TrainDetailsMap[levelName];

        for (auto& train : trains)
        {
            if (!IsSubLevelLoaded(train.SubLevelName) && train.SequenceObjects.size() > 0)
            {
                SubLevelUnloaded = true;
                return;
            }

            for (auto& sequenceObject : train.SequenceObjects)
            {
                if (TrainEffectType == TrainEffect::Strange)
                {
                    train.NewPlayRate = RandomizePlayRate(train.DefaultPlayRate);
                }

                const auto objName = sequenceObject->ObjName.ToString();
                if (objName == "Matinee")
                {
                    auto matinee = static_cast<Classes::USeqAct_Interp*>(sequenceObject);
                    if (!matinee)
                    {
                        continue;
                    }

                    matinee->PlayRate = applyNewPlayRate ? train.NewPlayRate : train.DefaultPlayRate;
                }
                else if (objName == "Float")
                {
                    auto variable = static_cast<Classes::USeqVar_Float*>(sequenceObject);
                    if (!variable)
                    {
                        continue;
                    }

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
