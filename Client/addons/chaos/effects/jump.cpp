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
    float JumpTimeDelayMax = 6.9f;
    float JumpTimeDelay = 1.0f;
    float JumpTimeActivatedAt = 0.0f;
    EJumpType JumpType;

public:
    Jump(const std::string& name, EJumpType jumpType)
    {
        Name = name;
        DisplayName = name;

        JumpType = jumpType;
    }

    void Start() override
    {
        JumpTimeDelay = RandomFloat(0.0f, JumpTimeDelayMax);
        JumpTimeActivatedAt = static_cast<float>(GetTickCount64());
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
        return (static_cast<float>(GetTickCount64()) - JumpTimeActivatedAt) / 1000 > JumpTimeDelay;
    }

    void ForceJump()
    {
        auto controller = Engine::GetPlayerController();
        controller->bDuck = false;
        controller->PlayerInput->Jump();

        JumpTimeActivatedAt = static_cast<float>(GetTickCount64());
        JumpTimeDelay = RandomFloat(0.0f, JumpTimeDelayMax);
    }
};

using JumpRandomly = Jump;
using JumpConstantly = Jump;

REGISTER_EFFECT(JumpRandomly, "Jump Randomly", EJumpType::Random);
REGISTER_EFFECT(JumpConstantly, "Jump Constantly", EJumpType::Constant);
