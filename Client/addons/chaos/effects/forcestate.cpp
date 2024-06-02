#pragma once

#include "../effect.h"

enum class ForcedState
{
    Walking,
    Crouching
};

class ForceState : public Effect
{
private:
    ForcedState ForcedStateType;

public:
    ForceState(const std::string& name, ForcedState forcedStateType)
    {
        Name = name;
        DisplayName = name;
        DurationType = EffectDuration::Short;

        ForcedStateType = forcedStateType;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();

        if (ForcedStateType == ForcedState::Walking)
        {
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
        else
        {
            controller->bDuck = TRUE;
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();
        
        if (ForcedStateType == ForcedState::Walking)
        {
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
        else
        {
            controller->bDuck = FALSE;
            return true;
        }
    }

    std::string GetType() const override
    {
        return "ForceState";
    }
};

using ForceWalking = ForceState;
using ForceCrouching = ForceState;

REGISTER_EFFECT(ForceWalking, "Force Walking", ForcedState::Walking);
REGISTER_EFFECT(ForceCrouching, "Force Crouching", ForcedState::Crouching);