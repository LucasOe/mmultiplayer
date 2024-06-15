#pragma once

#include "../effect.h"

enum class EHealth
{
    NoHealth,
    OneHitKnockOut
};

class Health : public Effect
{
private:
    EHealth HealthType;

public:
    Health(const std::string& name, EHealth healthType)
    {
        Name = name;
        DisplayName = name;

        HealthType = healthType;
    }

    void Start() override 
    {
        IsDone = false;
    }

    void Tick(const float deltaTime) override
    {
        if (IsDone)
        {
            return;
        }

        auto pawn = Engine::GetPlayerPawn();
        if (HealthType == EHealth::NoHealth)
        {
            if (pawn->Health <= 0)
            {
                IsDone = true;
                return;
            }

            pawn->Suicide();
        }
        else if (HealthType == EHealth::OneHitKnockOut)
        {
            if (pawn->Health > 1)
            {
                pawn->Health = 1;
            }
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "Health";
    }
};

using ZeroHealth = Health;
using OneHealth = Health;

REGISTER_EFFECT(ZeroHealth, "Suicide", EHealth::NoHealth);
REGISTER_EFFECT(OneHealth, "OHKO", EHealth::OneHitKnockOut);