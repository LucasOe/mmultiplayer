#include "effect.h"

std::vector<Effect*>& Effects()
{
    static std::vector<Effect*> effects;
    return effects;
}

int Effect::RandomInt(const int max) const
{
    std::uniform_int_distribution<int> dist(0, max);
    return dist(rng);
}

int Effect::RandomInt(const int min, const int max) const
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float Effect::RandomFloat(const float max) const
{
    std::uniform_real_distribution<float> dist(0.0f, max);
    return dist(rng);
}

float Effect::RandomFloat(const float min, const float max) const
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

Classes::UTdPlayerInput* Effect::GetTdPlayerInput()
{
    // We don't check for nullptr for the controller since when it's called from Tick() in chaos.cpp
    // The controller is not a nullptr
    const auto controller = Engine::GetPlayerController();

    if (!controller->PlayerInput)
    {
        return nullptr;
    }

    const auto input = static_cast<Classes::UTdPlayerInput*>(controller->PlayerInput);
    if (!input)
    {
        return nullptr;
    }

    return input;
}
