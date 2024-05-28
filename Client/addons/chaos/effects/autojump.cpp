#pragma once

#include "../effect.h"

enum class JumpType
{
    Random,
    Constant
};

template <JumpType JumpType>
class AutoJump : public Effect
{
private:
    float JumpTimeDelayMax = 6.9f;
    float JumpTimeDelay = 1.0f;
    float JumpTimeActivatedAt = 0.0f;

public:
    AutoJump(const std::string& name)
    {
        Name = name;
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

        if (JumpType == JumpType::Constant)
        {
            Jump();
        }
        else
        {
            if (!CanJump())
            {
                return;
            }

            Jump();
        }
    }

    bool Shutdown() override 
    {
        return true;
    }

    std::string GetType() override
    {
        return "AutoJump";
    }

private:
    bool CanJump()
    {
        return (static_cast<float>(GetTickCount64()) - JumpTimeActivatedAt) / 1000 > JumpTimeDelay;
    }

    void Jump()
    {
        JumpTimeActivatedAt = static_cast<float>(GetTickCount64());
        JumpTimeDelay = RandomFloat(JumpTimeDelayMax);
        Engine::GetPlayerController()->PlayerInput->Jump();
    }
};

using JumpRandomly = AutoJump<JumpType::Random>;
using JumpConstantly = AutoJump<JumpType::Constant>;

REGISTER_EFFECT(JumpRandomly, "Jump Randomly");
REGISTER_EFFECT(JumpConstantly, "Jump Constantly");
