#pragma once

#include "../effect.h"

enum class EForcedState
{
    Walking,
    Crouching
};

class ForceState : public Effect
{
private:
    EForcedState ForcedState;

public:
    ForceState(const std::string& name, EForcedState forcedState)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Short;

        ForcedState = forcedState;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto pawn = Engine::GetPlayerPawn();

        if (pawn->MovementState != Classes::EMovement::MOVE_Walking && pawn->MovementState != Classes::EMovement::MOVE_Crouch)
        {
            return;
        }

        auto turn180 = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_180Turn)]);
        if (turn180)
        {
            turn180->LastStopMoveTime = -1.0f;
        }

        auto crouchMelee = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_MeleeCrouch)]);
        if (crouchMelee)
        {
            crouchMelee->LastStopMoveTime = -1.0f;
        }

        auto controller = Engine::GetPlayerController();

        if (ForcedState == EForcedState::Walking)
        {
            auto input = GetTdPlayerInput();
            if (!input)
            {
                return;
            }

            input->bWalkButtonPressed = true;
            controller->bDuck = false;
        }
        else
        {
            controller->bDuck = true;
        }

        controller->bPressedJump = false;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto pawn = Engine::GetPlayerPawn();
        auto turn180 = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_180Turn)]);
        if (turn180)
        {
            turn180->LastStopMoveTime = 0.1f;
        }

        auto crouchMelee = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_MeleeCrouch)]);
        if (crouchMelee)
        {
            crouchMelee->LastStopMoveTime = 0.1f;
        }

        auto controller = Engine::GetPlayerController();
        if (ForcedState == EForcedState::Walking)
        {
            auto input = GetTdPlayerInput();

            if (!input)
            {
                return false;
            }

            input->bWalkButtonPressed = false;
            return true;
        }

        controller->bDuck = false;
        return true;
    }

    std::string GetType() const override
    {
        return "ForceState";
    }
};

using ForceWalking = ForceState;
using ForceCrouching = ForceState;

REGISTER_EFFECT(ForceWalking, "Force Walking", EForcedState::Walking);
REGISTER_EFFECT(ForceCrouching, "Force Crouching", EForcedState::Crouching);