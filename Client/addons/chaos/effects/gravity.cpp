#pragma once

#include "../effect.h"

class Gravity : public Effect
{
private:
    float GravityModifier = 0.0f;
    float NewAccelRate = 0.0f;
    float PreviousAccelRate = 6144.0f;

public:
    Gravity(const std::string& name, float gravityModifier)
    {
        Name = name;
        DisplayName = name;

        GravityModifier = gravityModifier;
    }

    void Initialize() override
    {
        const auto pawn = Engine::GetPlayerPawn();
        PreviousAccelRate = pawn->AccelRate;
        NewAccelRate = pawn->AccelRate * GravityModifier;
    }

    void Tick(const float deltaTime) override
    {
        auto pawn = Engine::GetPlayerPawn();
        auto world = Engine::GetWorld();
        
        if (!pawn || !world)
        {
            return;
        }

        pawn->AccelRate = NewAccelRate;
        world->WorldGravityZ = world->DefaultGravityZ * GravityModifier;
    }

    bool Shutdown() override
    {
        auto pawn = Engine::GetPlayerPawn();
        auto world = Engine::GetWorld();

        if (!pawn || !world)
        {
            return false;
        }

        pawn->AccelRate = PreviousAccelRate;
        world->WorldGravityZ = world->DefaultGravityZ;

        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Gravity;
    }

    std::string GetClass() const override
    {
        return "Gravity";
    }
};

using MoonGravity = Gravity;
using MarsGravity = Gravity;
using JupiterGravity = Gravity;
using PlutoGravity = Gravity;

// If it wasn't for the meme category of "Any% WITH SPACEBOOTS"
// These would be like the timescale values like half gravity, double gravity, and etc
// Other planets are not used since they were too similar to other ones
 
// Source for the float values, thank you Dave :)
// https://nssdc.gsfc.nasa.gov/planetary/factsheet/planet_table_ratio.html

REGISTER_EFFECT(MoonGravity, "Moon Gravity", 0.166f);
REGISTER_EFFECT(MarsGravity, "Mars Gravity", 0.377f);
REGISTER_EFFECT(JupiterGravity, "Jupiter Gravity", 2.36f);
REGISTER_EFFECT(PlutoGravity, "Pluto Gravity", 0.071f);
