#pragma once

#include "../effect.h"

enum class EHealth
{
    NoHealth,
    NoHealthRegeneration,
    OneHitKnockOut,
    MaxHealthPlayer,
    MaxHealthAI
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
        if (!pawn)
        {
            return;
        }

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
        else if (HealthType == EHealth::NoHealthRegeneration)
        {
            pawn->RegenerateDelay = 0.0f;
            pawn->RegenerateHealthPerSecond = 0.0f;
        }
        else if (HealthType == EHealth::MaxHealthPlayer)
        {
            pawn->Health = pawn->MaxHealth;
        }
        else if (HealthType == EHealth::MaxHealthAI)
        {
            auto aicontrollers = GetTdAIControllers();

            for (size_t i = 0; i < aicontrollers.Num(); i++)
            {
                auto ai = aicontrollers[i];

                if (!ai) continue;
                if (!ai->myPawn) continue;

                ai->myPawn->Health = ai->myPawn->MaxHealth;
            }
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        if (HealthType == EHealth::NoHealthRegeneration)
        {
            auto pawn = Engine::GetPlayerPawn();
            if (!pawn)
            {
                return false;
            }

            pawn->RegenerateDelay = 5.0f;
            pawn->RegenerateHealthPerSecond = 25.0f;
        }

        return true;
    }

    std::string GetType() const override
    {
        return "Health";
    }

    EGroup GetGroup() override
    {
        if (HealthType == EHealth::MaxHealthAI)
        {
            return EGroup_AI | EGroup_Health;
        }

        return EGroup_Player | EGroup_Health;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_None;
    }
};

using ZeroHealth = Health;
using OneHealth = Health;
using NoHealthRegeneration = Health;
using MaxHealthPlayer = Health;
using MaxHealthAI = Health;

REGISTER_EFFECT(ZeroHealth, "Suicide", EHealth::NoHealth);
REGISTER_EFFECT(OneHealth, "OHKO", EHealth::OneHitKnockOut);
REGISTER_EFFECT(NoHealthRegeneration, "No Health Regeneration", EHealth::NoHealthRegeneration);
REGISTER_EFFECT(MaxHealthPlayer, "Max Health (Player)", EHealth::MaxHealthPlayer);
REGISTER_EFFECT(MaxHealthAI, "Max Health (AI)", EHealth::MaxHealthAI);
