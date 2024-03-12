#pragma once

#include "json.h"

namespace Settings 
{
    void SetSetting(const std::vector<std::string> keys, const json value);
    json GetSetting(const std::vector<std::string> keys, json defaultValue);
    void Load();
    void Reset();
    void Save();
}