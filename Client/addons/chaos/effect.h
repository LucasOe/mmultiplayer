#pragma once

#include "../../sdk.h"
#include "../../engine.h"
#include "../../imgui/imgui_mmultiplayer.h"
#include <vector>
#include <random>

enum class EffectDuration
{
    Short,
    Normal,
    Long,
};

class Effect
{
// Inherited classes should not modify these but you can use them
public:
    bool IsEnabled = true;
    float DurationTimeAllocated = 0.0f;
    EffectDuration DurationType = EffectDuration::Normal;

// Varibles to use if your effect is does one thing and is done after success
public:
    bool IsDone = false;

// Variables and functions to override
public:
    std::string Name = "";
    std::string DisplayName = "";

    virtual void Start() = 0;
    virtual void Tick(const float deltaTime) = 0;
    virtual void Render(IDirect3DDevice9* device) = 0;
    virtual bool Shutdown() = 0;
    virtual std::string GetType() const = 0;

// Helper functions
public:
    // Min value is set to 0
    int RandomInt(const int max) const;
    int RandomInt(const int min, const int max) const;

    // Min value is set to 0.0f
    float RandomFloat(const float max) const;                   
    float RandomFloat(const float min, const float max) const;

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
static EFFECT_CLASS##_Register global_##EFFECT_CLASS##_register;
