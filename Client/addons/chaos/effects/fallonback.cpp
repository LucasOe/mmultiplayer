#pragma once

#include "../effect.h"

class FallOnBack : public Effect
{
private:
    float Time = 0.0f;
    float RandomDelay = 0.0f;
    float MaxDelay = 8.0f;

public:
    FallOnBack(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override
    {
        Time = 0.0f;
        RandomDelay = RandomFloat(MaxDelay);
    }

    void Tick(const float deltaTime) override
    {
        auto pawn = Engine::GetPlayerPawn();

        Time += deltaTime;

        if (Time < RandomDelay)
        {
            return;
        }

        pawn->OnTdFallOnBack(nullptr);

        Time = 0.0f;
        RandomDelay = RandomFloat(MaxDelay);
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
};

REGISTER_EFFECT(FallOnBack, "Fall On Back Randomly");
