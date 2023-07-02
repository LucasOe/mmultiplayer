#pragma once

#include "../addon.h"
#include "../engine.h"
#include "../sdk.h"
#include <string>
#include <vector>
#include <windows.h>


class Misc : public Addon {
  public:
    bool Initialize();
    std::string GetName();
};

enum class EOhko : uint8_t {
    Normal,
    Extreme,
};