#pragma once

#include "../effect.h"

class Nothing : public Effect
{
public:
    Nothing(const std::string& name)
    {
        Name = name;
    }

    void OnStart() override {}

    void OnTick(const float deltaTime) override {}

    void OnEnd() override {}
};

//REGISTER_EFFECT(Nothing, "Do Nothing");
