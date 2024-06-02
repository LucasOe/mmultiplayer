#pragma once

#include "../effect.h"

class DisableInput : public Effect
{
public:
    DisableInput(const std::string& name)
    {
        Name = name;
        DisplayName = name;
        DurationType = EffectDuration::Short;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();
        controller->bIgnoreButtonInput = TRUE;
        controller->bIgnoreMoveInput = TRUE;
        controller->bIgnoreLookInput = TRUE;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();
        controller->bIgnoreButtonInput = FALSE;
        controller->bIgnoreMoveInput = FALSE;
        controller->bIgnoreLookInput = FALSE;

        return true;
    }

    std::string GetType() const override
    {
        return "DisableInput";
    }
};

REGISTER_EFFECT(DisableInput, "Input Disabled");
