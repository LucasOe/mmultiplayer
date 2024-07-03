#pragma once

#include "../effect.h"

class Nothing : public Effect
{
private:
    std::vector<std::string> DisplayNames = {
        "Do Nothing",
        "Teleport To Current Location",
        "Give Current Health To Player",
        // Need more...
    };

public:
    Nothing(const std::string& name)
    {
        Name = name;
        DurationType = EDuration::Short;
    }

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override 
    {
        // For now this will just say "Do Nothing" until I have more it can pick from
        DisplayName = DisplayNames[0];
        Done = true;
    }

    void Tick(const float deltaTime) override {}

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override 
    {
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_None;
    }

    std::string GetClass() const override
    {
        return "Nothing";
    }
};

REGISTER_EFFECT(Nothing, "Does Nothing");
