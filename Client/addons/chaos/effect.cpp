#include "effect.h"

std::vector<Effect*>& Effects()
{
    static std::vector<Effect*> effects;
    return effects;
}