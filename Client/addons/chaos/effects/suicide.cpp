#pragma once

#include "../effect.h"

class Suicide : public Effect
{
public:
    Suicide(const std::string& name)
    {
        Name = name;
        DisplayName = name;
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

        const auto pawn = Engine::GetPlayerPawn();

        if (pawn->Health <= 0)
        {
            IsDone = true;
            return;
        }

        pawn->Suicide();
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "Suicide";
    }
};

REGISTER_EFFECT(Suicide, "Suicide");
