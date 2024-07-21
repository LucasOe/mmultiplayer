#pragma once

#include "../effect.h"

enum class EJumpType
{
    Random,
    Constant
};

class Jump : public Effect
{
private:
    EJumpType JumpType;
    float TimeDelay = 1.0f;
    float TimeActivatedAt = 0.0f;
    const float TimeDelayMax = 6.0f;

public:
    Jump(const std::string& name, EJumpType jumpType)
    {
        Name = name;
        DisplayName = name;

        JumpType = jumpType;
    }

    void Initialize() override 
    {
        TimeDelay = RandomFloat(0.0f, TimeDelayMax);
        TimeActivatedAt = static_cast<float>(GetTickCount64());
    }

    void Tick(const float deltaTime) override
    {
        if (JumpType == EJumpType::Constant)
        {
            ForceJump();
            return;
        }

        if (!CanJump())
        {
            return;
        }

        ForceJump();
    }

    EGroup GetGroup() override
    {
        return EGroup_Input | EGroup_Player;
    }

    std::string GetClass() const override
    {
        return "Jump";
    }

private:
    bool CanJump()
    {
        return (static_cast<float>(GetTickCount64()) - TimeActivatedAt) / 1000 > TimeDelay;
    }

    void ForceJump()
    {
        const auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (!pawn || !controller)
        {
            return;
        }

        if (!controller->PlayerInput)
        {
            return;
        }

        if (pawn->MovementState != Classes::EMovement::MOVE_Grabbing)
        {
            controller->bDuck = false;
        }

        controller->PlayerInput->Jump();

        TimeActivatedAt = static_cast<float>(GetTickCount64());
        TimeDelay = RandomFloat(0.0f, TimeDelayMax);
    }
};

using JumpRandomly = Jump;
using JumpConstantly = Jump;

REGISTER_EFFECT(JumpRandomly, "Jump Randomly", EJumpType::Random);
REGISTER_EFFECT(JumpConstantly, "Jump Constantly", EJumpType::Constant);
