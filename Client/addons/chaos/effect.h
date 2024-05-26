#pragma once

#include "../../sdk.h"
#include "../../engine.h"
#include "../../imgui/imgui_mmultiplayer.h"
#include <vector>

class Effect
{
public:
    std::string Name = "";

    virtual ~Effect() = default;
    virtual void OnStart() = 0;
    virtual void OnTick(const float deltaTime) = 0;
    virtual void OnEnd() = 0;

    static inline float RandomFloat(float max)
    {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * max;
    }
};

std::vector<Effect*>& Effects();

#define REGISTER_EFFECT(EFFECT_CLASS, ...)                            \
extern "C" Effect* Create##EFFECT_CLASS()                             \
{                                                                     \
    return new EFFECT_CLASS(__VA_ARGS__);                             \
}                                                                     \
struct EFFECT_CLASS##_Register                                        \
{                                                                     \
    EFFECT_CLASS##_Register()                                         \
    {                                                                 \
        Effects().push_back(Create##EFFECT_CLASS());                  \
    }                                                                 \
};                                                                    \
static EFFECT_CLASS##_Register global_##EFFECT_CLASS##_register;
