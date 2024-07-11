#pragma once

#include "../effect.h"
#include "../../dolly.h"

class CameraRotation : public Effect
{
private:
    bool DoRotateCamera = false;
    bool DoRotateClockwise = true;

public:
    CameraRotation(const std::string& name, bool doRotateCamera)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Short;

        DoRotateCamera = doRotateCamera;
    }

    void Initialize() override 
    {
        DoRotateClockwise = RandomBool();
    }

    void Tick(const float deltaTime) override
    {
        auto controller = Engine::GetPlayerController();

        Dolly dolly;
        dolly.ForceRoll(true);

        if (DoRotateCamera)
        {
            const float degreesPerSecond = 65536.0f / DurationTime;
            const float rotationIncrement = degreesPerSecond * deltaTime;

            const int roll = static_cast<int>(controller->Rotation.Roll % 0x10000);

            if (DoRotateClockwise)
            {
                controller->Rotation.Roll = roll + static_cast<int>(rotationIncrement);
            }
            else
            {
                controller->Rotation.Roll = roll - static_cast<int>(rotationIncrement);
            }
        }
        else
        {
            controller->Rotation.Roll = 0x8000;
        }
    }

    bool Shutdown() override
    {
        auto controller = Engine::GetPlayerController();
        controller->Rotation.Roll = 0;

        Dolly dolly;
        dolly.ForceRoll(false);

        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Camera;
    }

    std::string GetClass() const override
    {
        return "CameraRotation";
    }
};

using CameraRotateOnce = CameraRotation;
using CameraUpsideDown = CameraRotation;

// These effect might be too much...
// REGISTER_EFFECT(CameraRotateOnce, "Camera Rotate Once", true);
// REGISTER_EFFECT(CameraUpsideDown, "Camera Upside Down", false);
