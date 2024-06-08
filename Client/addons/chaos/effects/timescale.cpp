#pragma once

#include "../effect.h"

class Timescale : public Effect
{
private:
    float NewTimescale = 1.0f;

public:
    Timescale(const std::string& name, float timescale)
    {
        Name = name;
        DisplayName = name;

        NewTimescale = timescale;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto world = Engine::GetWorld();
        world->TimeDilation = NewTimescale;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto world = Engine::GetWorld();
        world->TimeDilation = 1.0f;

        return true;
    }

    std::string GetType() const override
    {
        return "Timescale";
    }
};

using TimescaleQuarter = Timescale;
using TimescaleHalf = Timescale;
using TimescaleDouble = Timescale;
using TimescaleQuad = Timescale; 

REGISTER_EFFECT(TimescaleQuarter, "0.25x Timescale", 0.25f);
REGISTER_EFFECT(TimescaleHalf, "0.5x Timescale", 0.5f);
REGISTER_EFFECT(TimescaleDouble, "2x Timescale", 2.0f);
REGISTER_EFFECT(TimescaleQuad, "4x Timescale", 4.0f);
