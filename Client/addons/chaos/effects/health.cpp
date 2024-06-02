#pragma once

#include "../effect.h"

class Health : public Effect
{
private:
    int NewHealthValue = 0;

public:
    Health(const std::string& name, int newHealth)
    {
        Name = name;
        DisplayName = name;

        NewHealthValue = newHealth;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto pawn = Engine::GetPlayerPawn();
        if (pawn->Health > 1)
        {
            pawn->Health = NewHealthValue;
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

using OneHealth = Health;

REGISTER_EFFECT(OneHealth, "OHKO", 1);