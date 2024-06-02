#pragma once

#include "../effect.h"

enum class JumpTypeEffect
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
    JumpTypeEffect JumpType;

public:
    Jump(const std::string& name, JumpTypeEffect jumpType)
    {
        Name = name;
        DisplayName = name;

        JumpType = jumpType;
    }

    void Start() override
    {
        JumpTimeDelay = RandomFloat(JumpTimeDelayMax);
        JumpTimeActivatedAt = static_cast<float>(GetTickCount64());
    }

    void Tick(const float deltaTime) override
    {
        auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (JumpType == JumpTypeEffect::Constant)
        {
            ForceJump();
        }
        else
        {
            if (!CanJump())
            {
                return;
            }

            ForceJump();
        }
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
        JumpTimeActivatedAt = static_cast<float>(GetTickCount64());
        JumpTimeDelay = RandomFloat(JumpTimeDelayMax);
        Engine::GetPlayerController()->PlayerInput->Jump();
    }
};

using JumpRandomly = Jump;
using JumpConstantly = Jump;

REGISTER_EFFECT(JumpRandomly, "Jump Randomly", JumpTypeEffect::Random);
REGISTER_EFFECT(JumpConstantly, "Jump Constantly", JumpTypeEffect::Constant);
