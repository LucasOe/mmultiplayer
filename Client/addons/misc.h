#pragma once

#include "../addon.h"
#include "../engine.h"
#include "../sdk.h"
#include <vector>

class Misc : public Addon 
{
  public:
    bool Initialize();
    std::string GetName();
};
