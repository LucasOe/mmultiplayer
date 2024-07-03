#pragma once

#include "../effect.h"

class CameraShake : public Effect
{
private:
    int Intensity = 0;

public:
    CameraShake(const std::string& name, int intensity)
    {
        Name = name;
        DisplayName = name;

        Intensity = intensity;
    }

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override {}

    void Tick(const float deltaTime) override
    {
        auto controller = Engine::GetPlayerController();
        controller->CurrentLookAtPoint = nullptr;

        controller->Rotation.Pitch += RandomInt(-Intensity, Intensity);
        controller->Rotation.Yaw += RandomInt(-Intensity, Intensity);
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Camera;
    }

    std::string GetClass() const override
    {
        return "CameraShake";
    }
};

using CameraShakeEffect = CameraShake;

REGISTER_EFFECT(CameraShakeEffect, "Camera Shake", 64);
