#pragma once

#include "../effect.h"

class ForceRandomDirection : public Effect
{
private:
    float Time = 0.0f;
    float RandomDelay = 0.0f;
    float MaxDelay = 8.0f;

public:
    ForceRandomDirection(const std::string& name)
    {
        Name = name;
    }

    void Start() override 
    {
        Time = 0.0f;
        RandomDelay = RandomFloat(MaxDelay);
    }

    void Tick(const float deltaTime) override 
    {
        auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        Time += deltaTime;

        // While this effect is active, you could hold down the lookat binding and avoid this effect.
        // To fix this, we set this variable to null and it fixes it. Basically, it does the same as "exec function LookAtRelease()" in TdPlayerController.uc
        controller->CurrentLookAtPoint = nullptr;

        if (Time < RandomDelay)
        {
            return;
        }

        const int newYawValue = RandomInt(0, 65535);
        controller->Rotation.Yaw = newYawValue;

        const float yawInRadians = (newYawValue / 65535.0f) * 2.0f * CONST_Pi;
        const float dirX = cos(yawInRadians);
        const float dirY = sin(yawInRadians);

        pawn->Velocity.X = dirX * pawn->VelocityMagnitude2D;
        pawn->Velocity.Y = dirY * pawn->VelocityMagnitude2D;

        RandomDelay = RandomFloat(MaxDelay);
        Time = 0.0f;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() override
    {
        return "ForceRandomDirection";
    }
};

REGISTER_EFFECT(ForceRandomDirection, "Force Random Direction");
