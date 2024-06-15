#include "effect.h"

std::vector<Effect*>& Effects()
{
    static std::vector<Effect*> effects;
    return effects;
}

int Effect::RandomInt(const int min, const int max) const
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float Effect::RandomFloat(const float min, const float max) const
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

bool Effect::RandomBool(const float probability) const
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) < ImClamp(probability, 0.0f, 1.0f);
}

Classes::UTdPlayerInput* Effect::GetTdPlayerInput()
{
    const auto controller = Engine::GetPlayerController();
    if (!controller)
    {
        return nullptr;
    }

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

Classes::TArray<Classes::ATdAIController*> Effect::GetTdAIControllers()
{
    const auto world = Engine::GetWorld();
    if (!world)
    {
        return Classes::TArray<Classes::ATdAIController*>();
    }

    const auto gameInfo = static_cast<Classes::ATdSPGame*>(world->Game); 
    if (!gameInfo)
    {
        return Classes::TArray<Classes::ATdAIController*>();
    }

    return gameInfo->AIManager->AIControllers;
}
