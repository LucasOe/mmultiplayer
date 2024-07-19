#pragma once

#include "../effect.h"

class Nothing : public Effect
{
public:
    Nothing(const std::string& name)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Short;
    }

    void Initialize() override 
    {
        Done = true;
    }

    EGroup GetGroup() override
    {
        return EGroup_None;
    }

    std::string GetClass() const override
    {
        return "Do Nothing";
    }
};

REGISTER_EFFECT(Nothing, "Do Nothing");
