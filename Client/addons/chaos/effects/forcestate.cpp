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

        auto controller = Engine::GetPlayerController();

        if (ForcedState == EForcedState::Walking)
        {
            auto turn180 = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_180Turn)]);
            if (turn180)
            {
                turn180->LastStopMoveTime = -1.0f;
            }

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
        auto controller = Engine::GetPlayerController();
        
        if (ForcedState == EForcedState::Walking)
        {
            auto pawn = Engine::GetPlayerPawn();

            auto turn180 = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_180Turn)]);
            if (turn180)
            {
                turn180->LastStopMoveTime = pawn->WorldInfo->TimeSeconds;
            }

            auto input = GetTdPlayerInput();
            if (!input)
            {
                return false;
            }

            input->bWalkButtonPressed = false;
        }

        controller->bDuck = false;
        return true;
    }

    std::string GetType() const override
    {
        return "ForceState";
    }

    EGroup GetGroup() override
    {
        return EGroup_Player | EGroup_Input;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_None;
    }
};

using ForceWalking = ForceState;
using ForceCrouching = ForceState;

REGISTER_EFFECT(ForceWalking, "Force Walking", EForcedState::Walking);
REGISTER_EFFECT(ForceCrouching, "Force Crouching", EForcedState::Crouching);