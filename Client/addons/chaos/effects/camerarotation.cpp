#pragma once

#include "../effect.h"
#include "../../dolly.h"

class CameraRotation : public Effect
{
private:
    bool Rotate = false;

public:
    CameraRotation(const std::string& name, bool rotate)
    {
        Name = name;
        DisplayName = name;
        DurationType = EffectDuration::Short;

        Rotate = rotate;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();

        if (!controller->PlayerCamera)
        {
            return;
        }

        Dolly dolly;
        dolly.ForceRoll(true);

        if (Rotate)
        {
            float degreesPerSecond = 65536.0f / DurationTimeAllocated;
            float rotationIncrement = degreesPerSecond * deltaTime;

            int roll = static_cast<int>(controller->Rotation.Roll % 0x10000);
            controller->Rotation.Roll = roll + rotationIncrement;
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

        if (!controller->PlayerCamera)
        {
            return false;
        }

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
using CameraUpdsideDown = CameraRotation;

REGISTER_EFFECT(CameraRotateOnce, "Camera Rotate Once", true);
REGISTER_EFFECT(CameraUpdsideDown, "Camera Updside Down", false);