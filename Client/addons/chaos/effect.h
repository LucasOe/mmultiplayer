#pragma once

#include "group.h"
#include "../../sdk.h"
#include "../../engine.h"
#include "../../imgui/imgui_mmultiplayer.h"
#include <vector>
#include <random>

enum class EDuration
{
    Breif,
    Short,
    Normal,
    Long,
    COUNT
};

class Effect
{
// Variables
public:
    bool Enabled = true;
    bool Done = false;
    bool LevelEffect = false;
    
    float DurationTime = 0.0f;
    EDuration DurationType = EDuration::Normal;

    std::string Name = "";
    std::string DisplayName = "";

// Functions to override
public:
    // Return if the effect can be activated. Mainly used for level effects, if your effect isn't a level effect, return true
    // Example: If the current levelname is equal to convoy_p and the right sublevel is loaded
    //
    // This also makes the seeding useless but there's no 
    virtual bool CanActivate() = 0;

    // Initialize variables that use RandomX() functions
    virtual void Initialize() = 0;

    // Called on every tick if the pawn and controller exists
    // Check chaos.cpp Tick() function to see when this effect's Tick() get called 
    virtual void Tick(const float deltaTime) = 0;

    // Called on every tick even if pawn or controller doesn't exits
    // Used for rendering ImGui. Check imgui_demo.cpp on how to use it
    virtual void Render(IDirect3DDevice9* device) = 0;

    // Restore what you modified to the original/default state and return if it did restore correctly
    virtual bool Shutdown() = 0;

    // Displayed in the UI which group the effect belongs in
    virtual EGroup GetGroup() = 0;

    // Returns the classname, this is done so no duplicate effects can happend
    // Example: 0.25x Timescale would not activate if 2x Timescale is active
    virtual std::string GetClass() const = 0;

// Helper functions
protected:
    int RandomInt(const int min, const int max) const;               
    float RandomFloat(const float min, const float max) const;
    bool RandomBool(const float probability = 0.5f) const;

// Custom helper functions
protected:
    Classes::UTdPlayerInput* GetTdPlayerInput();
    Classes::TArray<Classes::ATdAIController*> GetTdAIControllers();

    // Gets the sequence objects from the level and the position where the kismet node is
    // NOTE: This will not work if the level's kismet has been moved around
    std::vector<Classes::USequenceObject*> GetKismetSequenceObjects(const std::string& levelName, const ImVec2& pos) const;

    // Gets the level name in lowercase. If it failed, the return will be an empty string
    std::string GetLevelName() const;

// Seeding, you don't need to do anything in your effect with these
public:
    mutable std::mt19937 rng;
    void SetSeed(const int newSeed) const
    {
        rng.seed(newSeed);
    }
};

std::vector<Effect*>& Effects();

#define REGISTER_EFFECT(EFFECT_CLASS, ...)                              \
extern "C" Effect* Create##EFFECT_CLASS()                               \
{                                                                       \
    return new EFFECT_CLASS(__VA_ARGS__);                               \
}                                                                       \
struct EFFECT_CLASS##_Register                                          \
{                                                                       \
    EFFECT_CLASS##_Register()                                           \
    {                                                                   \
        Effects().push_back(Create##EFFECT_CLASS());                    \
    }                                                                   \
};                                                                      \
static EFFECT_CLASS##_Register Global_##EFFECT_CLASS##_Register;
