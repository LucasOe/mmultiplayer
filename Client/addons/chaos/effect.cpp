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
