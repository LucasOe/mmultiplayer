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

    void Start() override 
    {
        DoRotateClockwise = RandomBool();
    }

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();

        Dolly dolly;
        dolly.ForceRoll(true);

        if (DoRotateCamera)
        {
            float degreesPerSecond = 65536.0f / DurationTime;
            float rotationIncrement = degreesPerSecond * deltaTime;

            int roll = static_cast<int>(controller->Rotation.Roll % 0x10000);

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

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();
        controller->Rotation.Roll = 0;

        Dolly dolly;
        dolly.ForceRoll(false);

        return true;
    }

    std::string GetType() const override
    {
        return "CameraRotation";
    }
};

using CameraRotateOnce = CameraRotation;
using CameraUpsideDown = CameraRotation;

REGISTER_EFFECT(CameraRotateOnce, "Camera Rotate Once", true);
REGISTER_EFFECT(CameraUpsideDown, "Camera Upside Down", false);