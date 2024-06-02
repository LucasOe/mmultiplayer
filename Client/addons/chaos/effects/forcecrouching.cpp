#pragma once

#include "../effect.h"

class ForceCrouching : public Effect
{
public:
    ForceCrouching(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();
        controller->bDuck = TRUE;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();
        controller->bDuck = FALSE;

        return true;
    }

    std::string GetType() const override
    {
        return "Force";
    }
};

REGISTER_EFFECT(ForceCrouching, "Force Crouching");