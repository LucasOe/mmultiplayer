#pragma once

#include "../effect.h"

class DisableInput : public Effect
{
private:
    bool PlayedIdleAnimation = false;

public:
    DisableInput(const std::string& name)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Breif;
    }

    void Initialize() override 
    {
        PlayedIdleAnimation = false;
    }

    void Tick(const float deltaTime) override
    {
        const auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (!pawn || !controller)
        {
            return;
        }

        controller->bIgnoreButtonInput = TRUE;
        controller->bIgnoreMoveInput = TRUE;
        controller->bIgnoreLookInput = TRUE;
        controller->CurrentLookAtPoint = nullptr;

        if (PlayedIdleAnimation || pawn->MovementState != Classes::EMovement::MOVE_Walking)
        {
            return;
        }

        auto walking = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_Walking)]);
        if (!walking)
        {
            return;
        }

        walking->TriggerIdleAnimMinTime = 0.0f;
        walking->TriggerIdleAnimMaxTime = 0.5f;
        walking->PlayIdle();

        PlayedIdleAnimation = walking->bIsPlayingIdleAnim;
    }

    bool Shutdown() override
    {
        const auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (!pawn || !controller)
        {
            return false;
        }

        controller->bIgnoreButtonInput = FALSE;
        controller->bIgnoreMoveInput = FALSE;
        controller->bIgnoreLookInput = FALSE;

        auto walking = static_cast<Classes::UTdMove_Walking*>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_Walking)]);
        if (!walking)
        {
            return false;
        }

        walking->TriggerIdleAnimMinTime = 30.0f;
        walking->TriggerIdleAnimMaxTime = 40.0f;

        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Disable | EGroup_Input | EGroup_Mouse;
    }

    std::string GetClass() const override
    {
        return "DisableInput";
    }
};

REGISTER_EFFECT(DisableInput, "Input Disabled");
