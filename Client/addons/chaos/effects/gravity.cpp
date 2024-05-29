#pragma once

#include "../effect.h"

class Gravity : public Effect
{
private:
    float GravityModifier = 0.0f;

public:
    Gravity(const std::string& name, float gravityModifier)
    {
        Name = name;
        GravityModifier = gravityModifier;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto world = Engine::GetWorld();
        world->WorldGravityZ = world->DefaultGravityZ * GravityModifier;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto world = Engine::GetWorld();
        world->WorldGravityZ = world->DefaultGravityZ;

        return true;
    }

    std::string GetType() override
    {
        return "Gravity";
    }
};

// Commented out a few since there are some that have close values to eachother
// I'll leave them here until I decide what I should do for these

//using MercuryGravity = Gravity;
//using VenusGravity = Gravity;
using MoonGravity = Gravity;
using MarsGravity = Gravity;
using JupiterGravity = Gravity;
//using SaturnGravity = Gravity;
//using UranusGravity = Gravity;
using NeptuneGravity = Gravity;
using PlutoGravity = Gravity;
 
// Source for the float values, thank you Dave :)
// https://nssdc.gsfc.nasa.gov/planetary/factsheet/planet_table_ratio.html

//REGISTER_EFFECT(MercuryGravity, "Mercury Gravity", 0.378f);
//REGISTER_EFFECT(VenusGravity, "Venus Gravity", 0.907f);
REGISTER_EFFECT(MoonGravity, "Moon Gravity", 0.166f);
REGISTER_EFFECT(MarsGravity, "Mars Gravity", 0.377f);
REGISTER_EFFECT(JupiterGravity, "Jupiter Gravity", 2.36f);
//REGISTER_EFFECT(SaturnGravity, "Saturn Gravity", 0.916f);
//REGISTER_EFFECT(UranusGravity, "Uranus Gravity", 0.889f);
REGISTER_EFFECT(NeptuneGravity, "Neptune Gravity", 1.12f);
REGISTER_EFFECT(PlutoGravity, "Pluto Gravity", 0.071f);
