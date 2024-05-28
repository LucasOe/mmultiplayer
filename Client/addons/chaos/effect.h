#pragma once

#include "../../sdk.h"
#include "../../engine.h"
#include "../../imgui/imgui_mmultiplayer.h"
#include <vector>

class Effect
{
public:
    float TimeLeftInSeconds = 0.0f; // This is here for now until I find a better way
    std::string Name = "";

    virtual void Start() = 0;
    virtual void Tick(const float deltaTime) = 0;
    virtual bool Shutdown() = 0;
    virtual std::string GetType() = 0;

    // >> Helper functions 
    // TODO: Find better a better way generate these numbers 

    static int RandomInt(const int min, const int max)
    {
        return rand() % (max - min + 1) + min;
    }

    static float RandomFloat(const float max)
    {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * max;
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
