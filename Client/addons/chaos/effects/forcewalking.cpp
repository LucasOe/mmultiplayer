#pragma once

#include "../effect.h"

class ForceWalking : public Effect
{
public:
    ForceWalking(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();

        if (!controller->PlayerInput)
        {
            return;
        }

        auto input = static_cast<Classes::UTdPlayerInput*>(controller->PlayerInput);
        if (!input)
        {
            return;
        }

        input->bWalkButtonPressed = TRUE;
        controller->bPressedJump = FALSE;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();
        if (!controller->PlayerInput)
        {
            return false;
        }

        auto input = static_cast<Classes::UTdPlayerInput*>(controller->PlayerInput);
        if (!input)
        {
            return false;
        }

        input->bWalkButtonPressed = FALSE;
        return true;
    }

    std::string GetType() const override
    {
        return "Force";
    }
};

REGISTER_EFFECT(ForceWalking, "Force Walking");