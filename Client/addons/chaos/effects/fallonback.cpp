#pragma once

#include "../effect.h"

class FallOnBack : public Effect
{
private:
    float Time = 0.0f;
    float MinDelay = 2.0f;
    float MaxDelay = 10.0f;

public:
    FallOnBack(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Initialize() override 
    {
        Time = RandomFloat(MinDelay, MaxDelay);
    }

    void Tick(const float deltaTime) override
    {
        Time -= deltaTime;

        if (Time >= 0.0f)
        {
            return;
        }

        const auto pawn = Engine::GetPlayerPawn();
        if (!pawn)
        {
            return;
        }

        Time = RandomFloat(MinDelay, MaxDelay);
        pawn->OnTdFallOnBack(nullptr);
    }

    EGroup GetGroup() override
    {
        return EGroup_Player;
    }

    std::string GetClass() const override
    {
        return "FallOnBack";
    }
};

REGISTER_EFFECT(FallOnBack, "Fall On Back Randomly");
