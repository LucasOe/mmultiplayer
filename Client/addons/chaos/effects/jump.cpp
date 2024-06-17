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

    void Start() override
    {
        if (JumpType == EJumpType::Constant)
        {
            return;
        }

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

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override 
    {
        return true;
    }

    std::string GetType() const override
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
        auto controller = Engine::GetPlayerController();
        controller->bDuck = false;
        controller->PlayerInput->Jump();

        if (JumpType == EJumpType::Constant)
        {
            return;
        }

        TimeActivatedAt = static_cast<float>(GetTickCount64());
        TimeDelay = RandomFloat(0.0f, TimeDelayMax);
    }
};

using JumpRandomly = Jump;
using JumpConstantly = Jump;

REGISTER_EFFECT(JumpRandomly, "Jump Randomly", EJumpType::Random);
REGISTER_EFFECT(JumpConstantly, "Jump Constantly", EJumpType::Constant);
