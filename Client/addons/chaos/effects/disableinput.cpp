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

    void Start() override 
    {
        PlayedIdleAnimation = false;
    }

    void Tick(const float deltaTime) override
    {
        const auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

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

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

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

    std::string GetType() const override
    {
        return "DisableInput";
    }

    EGroup GetGroup() override
    {
        return EGroup_Disable | EGroup_Input | EGroup_Mouse;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_Input | EGroup_Mouse;
    }
};

REGISTER_EFFECT(DisableInput, "Input Disabled");
