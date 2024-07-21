#pragma once

#include "../effect.h"

class ForceRandomDirection : public Effect
{
private:
    float Time = 0.0f;
    float RandomDelay = 0.0f;
    float MinDelay = 1.0f;
    float MaxDelay = 8.0f;

public:
    ForceRandomDirection(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Initialize() override 
    {
        Time = 0.0f;
        RandomDelay = RandomFloat(MinDelay, MaxDelay);
    }

    void Tick(const float deltaTime) override 
    {
        auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (!pawn || !controller)
        {
            return;
        }

        controller->CurrentLookAtPoint = nullptr;

        Time += deltaTime;

        if (Time < RandomDelay)
        {
            return;
        }

        const int newYawValue = RandomInt(0, 65536);
        controller->Rotation.Yaw = newYawValue;

        const float yawInRadians = (newYawValue / 65536.0f) * CONST_Tau;
        const float dirX = cos(yawInRadians);
        const float dirY = sin(yawInRadians);

        pawn->Velocity.X = dirX * pawn->VelocityMagnitude2D;
        pawn->Velocity.Y = dirY * pawn->VelocityMagnitude2D;

        RandomDelay = RandomFloat(MinDelay, MaxDelay);
        Time = 0.0f;
    }

    EGroup GetGroup() override
    {
        return EGroup_Camera | EGroup_Player;
    }

    std::string GetClass() const override
    {
        return "ForceRandomDirection";
    }
};

REGISTER_EFFECT(ForceRandomDirection, "Force Random Direction");
