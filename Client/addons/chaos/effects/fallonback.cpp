#pragma once

#include "../effect.h"

class FallOnBack : public Effect
{
private:
    float Time = 0.0f;
    const float MinDelay = 3.0f;
    const float MaxDelay = 12.0f;

public:
    FallOnBack(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override
    {
        RandomizeDelay();
    }

    void Tick(const float deltaTime) override
    {
        const auto pawn = Engine::GetPlayerPawn();

        Time -= deltaTime;

        if (Time >= 0.0f)
        {
            return;
        }

        if (pawn->MovementState == Classes::EMovement::MOVE_Falling || 
            pawn->MovementState == Classes::EMovement::MOVE_FallingUncontrolled)
        {
            RandomizeDelay();
            return;
        }

        pawn->OnTdFallOnBack(nullptr);
        RandomizeDelay();
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "FallOnBack";
    }

private:
    void RandomizeDelay()
    {
        Time = RandomFloat(MinDelay, MaxDelay);
    }
};

REGISTER_EFFECT(FallOnBack, "Fall On Back Randomly");
