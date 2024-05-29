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
 
        controller->Rotation.Yaw = RandomInt(0, 65535);

        // pawn->VelocityDir2D.X;
        // pawn->VelocityDir2D.Y;
        // 
        // Found in UnMath.h (INT is a custom typedef which is a int32_t aka int)
        // class FRotator
        // {
        // public:
        //     // Variables.
        //     INT Pitch; // Looking up and down (0=Straight Ahead, +Up, -Down).
        //     INT Yaw;   // Rotating around (running in circles), 0=East, +North, -South.
        //     INT Roll;  // Rotation about axis of screen, 0=Straight, +Clockwise, -CCW.
        // }
        // 
        //              N       W       S       E
        //              16384   32767   49151   0
        // Dir2D X =    0f      -1f     0f      1f
        // Dir2D Y =    1f      0f      -1f     0f
        //
        // TODO: Figure out how to change the velocity to the new direction with help of VelocityDir2D variable
        // The float range of Velocity is -3500 to 3500
        // 
        // pawn->Velocity.X = ??;
        // pawn->Velocity.Y = ??;

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
