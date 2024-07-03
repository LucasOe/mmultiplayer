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

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override 
    {
        const auto pawn = Engine::GetPlayerPawn();
        PreviousAccelRate = pawn->AccelRate;
        NewAccelRate = pawn->AccelRate * GravityModifier;
    }

    void Tick(const float deltaTime) override
    {
        auto world = Engine::GetWorld();
        if (!world)
        {
            return;
        }
        world->WorldGravityZ = world->DefaultGravityZ * GravityModifier;
        
        auto pawn = Engine::GetPlayerPawn();
        pawn->AccelRate = NewAccelRate;

        // TODO? 
        // Change stuff that is affected by gravity such as the wallrunning, swingjumps, and etc.
        // 
        // This would make it harder or easier but not sure if it's worth doing
        // Changing the gravity and the accelrate is good enough
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        auto world = Engine::GetWorld();
        if (!world)
        {
            return false;
        }
        world->WorldGravityZ = world->DefaultGravityZ;

        auto pawn = Engine::GetPlayerPawn();
        pawn->AccelRate = PreviousAccelRate;

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
