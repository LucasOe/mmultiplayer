#pragma once

#include "../addon.h"

class Tag : public Addon {
  public:
    bool Initialize();
    std::string GetName();
};