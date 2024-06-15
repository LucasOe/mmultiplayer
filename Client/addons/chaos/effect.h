#pragma once

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

// Work in progress to replace std::string GetType()
enum class EGroup
{
    EGroup_None             = 0,
    EGroup_Input            = 1 << 0,
    EGroup_Camera           = 1 << 1,
    EGroup_GameSpeed        = 1 << 2,
    EGroup_Hud              = 1 << 3,
    EGroup_Commands         = 1 << 4,
    EGroup_Teleport         = 1 << 5,
    EGroup_All              = 0x7FFFFFFF
};

class Effect
{
// Inherited classes should not modify these but you can use them
public:
    float DurationTime = 0.0f;
    EDuration DurationType = EDuration::Normal;

// Variables to use and functions to override
public:
    bool IsEnabled = true;
    bool IsDone = false;
    std::string Name = "";
    std::string DisplayName = "";

    virtual void Start() = 0;
    virtual void Tick(const float deltaTime) = 0;
    virtual void Render(IDirect3DDevice9* device) = 0;
    virtual bool Shutdown() = 0;
    virtual std::string GetType() const = 0;

// Helper functions
protected:
    int RandomInt(const int min, const int max) const;               
    float RandomFloat(const float min, const float max) const;
    bool RandomBool(const float probability = 0.5f) const;

    Classes::UTdPlayerInput* GetTdPlayerInput();
    Classes::TArray<Classes::ATdAIController*> GetTdAIControllers();

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
