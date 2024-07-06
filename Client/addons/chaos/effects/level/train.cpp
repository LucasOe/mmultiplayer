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
    std::vector<Classes::USequenceObject*> SequenceObjects;
};

class Train : public Effect
{
private:
    float PlayerSpawnedTime = -1.0f;
    float TimeSinceLastChecked = 0.0f;
    TrainEffect TrainEffectType = TrainEffect::None;

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
        return GetTrains().size() > 0;
    }

    void Initialize() override
    {
        PlayerSpawnedTime = -1.0f;
        TimeSinceLastChecked = 0.0f;
    }

    void Tick(const float deltaTime) override
    {
        auto pawn = Engine::GetPlayerPawn();

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
            ModifyTrains(true);
        }

        // Don't check each tick, instead check every 1 second
        if (TimeSinceLastChecked >= 1.0f)
        {
            TimeSinceLastChecked = 0.0f;
            ModifyTrains(true);
        }
    }

    void Render(IDirect3DDevice9* device) override {}

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
            return RandomFloat(defaultPlayRate * -2.0f, defaultPlayRate * 2.0f);
        }

        switch (TrainEffectType)
        {
            case TrainEffect::Slow:
                return RandomFloat(defaultPlayRate / 4.0f, defaultPlayRate / 2.0f);
            case TrainEffect::Fast:
                return RandomFloat(defaultPlayRate * 2.0f, defaultPlayRate * 4.0f);
            case TrainEffect::None:
            default:
                return defaultPlayRate;
        }
    }

    void FindTrain(std::vector<TrainDetail>& trains, const std::string& levelName, const ImVec2& pos, float defaultPlayRate)
    {
        TrainDetail train;
        train.DefaultPlayRate = defaultPlayRate;
        train.SequenceObjects = GetKismetSequenceObjects(levelName, pos);

        trains.push_back(train);
    }

    std::vector<TrainDetail> GetTrains()
    {
        std::vector<TrainDetail> trains;
        const auto levelName = GetLevelName();

        if (levelName == "escape_p")
        {
            // These trains have no collision and is just for shows
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

        return trains;
    }

    void ModifyTrains(const bool applyNewPlayRate)
    {
        auto trains = GetTrains();

        for (size_t i = 0; i < trains.size(); i++)
        {
            if (!applyNewPlayRate)
            {
                trains[i].NewPlayRate = 1.0f;
            }

            for (size_t j = 0; j < trains[i].SequenceObjects.size(); j++)
            {
                const auto objName = trains[i].SequenceObjects[j]->ObjName.ToString();

                if (trains[i].NewPlayRate == 1.0f || TrainEffectType == TrainEffect::Strange)
                {
                    trains[i].NewPlayRate = RandomizePlayRate(trains[i].DefaultPlayRate);
                }

                if (objName == "Matinee")
                {
                    auto train = static_cast<Classes::USeqAct_Interp*>(trains[i].SequenceObjects[j]);
                    if (!train)
                    {
                        continue;
                    }

                    train->PlayRate = applyNewPlayRate ? trains[i].NewPlayRate : trains[i].DefaultPlayRate;
                }
                else if (objName == "Float")
                {
                    auto variable = static_cast<Classes::USeqVar_Float*>(trains[i].SequenceObjects[j]);
                    if (!variable)
                    {
                        continue;
                    }

                    variable->FloatValue = applyNewPlayRate ? trains[i].NewPlayRate : trains[i].DefaultPlayRate;
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
