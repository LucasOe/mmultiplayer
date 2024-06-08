#pragma once

#include "../effect.h"

enum class EForcedStateType
{
    Walking,
    Crouching
};

class ForceState : public Effect
{
private:
    EForcedStateType ForcedStateType;

public:
    ForceState(const std::string& name, EForcedStateType forcedState)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Short;

        ForcedStateType = forcedState;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();

        if (ForcedStateType == EForcedStateType::Walking)
        {
            const auto input = GetTdPlayerInput();
            if (!input)
            {
                return;
            }

            input->bWalkButtonPressed = TRUE;
            controller->bDuck = FALSE;
        }
        else
        {
            controller->bDuck = TRUE;
        }

        controller->bPressedJump = FALSE;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();
        
        if (ForcedStateType == EForcedStateType::Walking)
        {
            const auto input = GetTdPlayerInput();

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

REGISTER_EFFECT(ForceWalking, "Force Walking", EForcedStateType::Walking);
REGISTER_EFFECT(ForceCrouching, "Force Crouching", EForcedStateType::Crouching);