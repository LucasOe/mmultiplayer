#pragma once

#include "../effect.h"

class GameSpeed : public Effect
{
private:
    float NewTimeDilation = 1.0f;

public:
    GameSpeed(const std::string& name, float timeDilation)
    {
        Name = name;
        DisplayName = name;

        NewTimeDilation = timeDilation;
    } 

    void Tick(const float deltaTime) override
    {
        auto world = Engine::GetWorld();
        auto controller = Engine::GetPlayerController();

        if (!world || !controller)
        {
            return;
        }

        world->TimeDilation = NewTimeDilation;
        controller->bReactionTime = false;
        controller->ReactionTimeEnergy = 0.0f;
    }

    bool Shutdown() override
    {
        auto world = Engine::GetWorld();

        if (!world)
        {
            return false;
        }

        world->TimeDilation = 1.0f;
        
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_GameSpeed;
    }

    std::string GetClass() const override
    {
        return "GameSpeed";
    }
};

using GameSpeedQuarter = GameSpeed;
using GameSpeedHalf = GameSpeed;
using GameSpeedDouble = GameSpeed;
using GameSpeedQuad = GameSpeed;

REGISTER_EFFECT(GameSpeedQuarter, "0.25x GameSpeed", 0.25f);
REGISTER_EFFECT(GameSpeedHalf, "0.5x GameSpeed", 0.5f);
REGISTER_EFFECT(GameSpeedDouble, "2x GameSpeed", 2.0f);
REGISTER_EFFECT(GameSpeedQuad, "4x GameSpeed", 4.0f);
