#pragma once

#include "../effect.h"

class CameraShake : public Effect
{
private:
    int Intensity = 0;

public:
    CameraShake(const std::string& name, const int intensity)
    {
        Name = name;
        Intensity = intensity;
    }

    void Start() override {}

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

    std::string GetType() override
    {
        return "CameraShake";
    }
};

using CameraShakeEffect = CameraShake;

REGISTER_EFFECT(CameraShakeEffect, "Camera Shake", 64);
