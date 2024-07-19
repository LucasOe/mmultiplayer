#pragma once

#include "../../addon.h"

class Chaos : public Addon 
{
public:
    bool Initialize();
    std::string GetName();
};

// Global variable, used for effects to get the level name
extern std::string LevelName;
