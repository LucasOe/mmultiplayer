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

    std::string GetType() const override
    {
        return "CameraShake";
    }

    EGroup GetGroup() override
    {
        return EGroup_Camera;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_None;
    }
};

using CameraShakeEffect = CameraShake;

REGISTER_EFFECT(CameraShakeEffect, "Camera Shake", 64);
