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

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override {}

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
            auto turn180 = static_cast<Classes::UTdMove_180Turn*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_180Turn)]);
            if (turn180)
            {
                turn180->LastStopMoveTime = FLT_MAX;
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
            auto meleeCrouch = static_cast<Classes::UTdMove_MeleeCrouch*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_Crouch)]); 
            if (meleeCrouch)
            {
                if (meleeCrouch->MeleeState.GetValue() > Classes::EMeleeState::MS_MeleeAttackFinishing)
                {
                    meleeCrouch->MeleeState = Classes::EMeleeState::MS_MeleeAttackNormal;
                }
            }

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

            auto turn180 = static_cast<Classes::UTdMove_180Turn*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_180Turn)]);
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

    EGroup GetGroup() override
    {
        return EGroup_Player | EGroup_Input;
    }

    std::string GetClass() const override
    {
        return "ForceState";
    }
};

using ForceWalking = ForceState;
using ForceCrouching = ForceState;

REGISTER_EFFECT(ForceWalking, "Force Walking", EForcedState::Walking);
REGISTER_EFFECT(ForceCrouching, "Force Crouching", EForcedState::Crouching);
