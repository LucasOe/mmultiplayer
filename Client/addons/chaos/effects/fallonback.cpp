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

    void Start() override
    {
        RandomizeDelay();
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

        RandomizeDelay();
        pawn->OnTdFallOnBack(nullptr);
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
