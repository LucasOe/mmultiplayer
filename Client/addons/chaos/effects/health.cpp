#pragma once

#include "../effect.h"

enum class EHealth
{
    NoHealth,
    OneHitKnockOut,
    MaxHealth
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
        Done = false;
    }

    void Tick(const float deltaTime) override
    {
        if (Done)
        {
            return;
        }

        auto pawn = Engine::GetPlayerPawn();
        if (HealthType == EHealth::NoHealth)
        {
            if (pawn->Health <= 0)
            {
                Done = true;
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
        else if (HealthType == EHealth::MaxHealth)
        {
            pawn->Health = pawn->MaxHealth;
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

    EGroup GetGroup() override
    {
        return EGroup_Player;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_None;
    }
};

using ZeroHealth = Health;
using OneHealth = Health;
using MaxHealth = Health;

REGISTER_EFFECT(ZeroHealth, "Suicide", EHealth::NoHealth);
REGISTER_EFFECT(OneHealth, "OHKO", EHealth::OneHitKnockOut);
REGISTER_EFFECT(MaxHealth, "Max Health", EHealth::MaxHealth);
