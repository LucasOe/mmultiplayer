#pragma once

#include "../../effect.h"

// This effect is for the chapter 4 trains
// If more trains is added later, this effect needs to be rewritten to support it
// Since those these chapter 4 trains have playrate of 1.5f

class Train : public Effect
{
private:
    float DefaultPlayRate = 1.5f;
    float NewPlayRate = 1.5f;
    float PlayerSpawnedTime = -1.0f;
    float TimeSinceLastChecked = 0.0f;

public:
    Train(const std::string& name)
    {
        Name = name;
        DurationType = EDuration::Long;
        LevelEffect = true;
    }

    bool CanActivate() override
    {
        return GetLevelName() == "subway_p";
    }

    void Initialize() override
    {
        // 10% chance of trains having random playrate every 1 second
        if (RandomBool(0.10f))
        {
            DisplayName = "Weird Trains";
            NewPlayRate = -1.0f;
            return;
        }

        // 50% chance of either being slow or fast
        if (RandomBool())
        {
            DisplayName = "Slow Trains";
            NewPlayRate = RandomFloat(0.1f, 0.5f);
            return;
        }
        
        DisplayName = "Rush Hour";
        NewPlayRate = RandomFloat(3.5f, 7.0f);
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
            ModifyTrains(NewPlayRate);
        }

        // Don't check each tick, instead check every 1 second
        if (TimeSinceLastChecked >= 1.0f)
        {
            TimeSinceLastChecked = 0.0f;
            ModifyTrains(NewPlayRate);
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        ModifyTrains(DefaultPlayRate);
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
    std::vector<Classes::USequenceObject*> GetTrains()
    {
        std::vector<Classes::USequenceObject*> trains;

        const auto trainsBeforeJanitorRoom = GetKismetSequenceObjects("Subway_Plat_Spt", ImVec2(-464, 304));
        const auto trainsAfterJanitorRoom = GetKismetSequenceObjects("Subway_Tunnel_Spt", ImVec2(-464, 304));

        trains.insert(trains.end(), trainsBeforeJanitorRoom.begin(), trainsBeforeJanitorRoom.end());
        trains.insert(trains.end(), trainsAfterJanitorRoom.begin(), trainsAfterJanitorRoom.end());

        return trains;
    }

    void ModifyTrains(float playRate)
    {
        auto trains = GetTrains();

        if (playRate == -1.0f)
        {
            playRate = RandomFloat(DefaultPlayRate * -4, DefaultPlayRate * 4);
        }

        for (size_t i = 0; i < trains.size(); i++)
        {
            auto train = static_cast<Classes::USeqAct_Interp*>(trains[i]);
            if (!train)
            {
                continue;
            }

            train->PlayRate = playRate;
        }
    }
};

REGISTER_EFFECT(Train, "Train PlayRate Randomized");
