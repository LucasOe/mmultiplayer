#pragma once

#include "../effect.h"

class Nothing : public Effect
{
public:
    Nothing(const std::string& name)
    {
        Name = name;
    }

    void Start() override {}

    void Tick(const float deltaTime) override {}

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override 
    {
        return true;
    }

    std::string GetType() override
    {
        return "Nothing";
    }
};

//REGISTER_EFFECT(Nothing, "Do Nothing");
