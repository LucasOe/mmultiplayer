#pragma once

#include "../effect.h"

class Fov : public Effect
{
private:
    float NewFov = 100.0f;
public:
    Fov(const std::string& name, const float fov)
    {
        Name = name;
        NewFov = fov;
    }

    void Start() override {}

    void Tick(const float deltaTime) override 
    {
        auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (controller->PlayerCamera)
        {
            controller->PlayerCamera->SetFOV(NewFov);
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override 
    {
        auto pawn = Engine::GetPlayerPawn();
        auto controller = Engine::GetPlayerController();

        if (controller->PlayerCamera)
        {
            controller->PlayerCamera->SetFOV(controller->PlayerCamera->DefaultFOV);
            return true;
        }

        return false;
    }

    std::string GetType() override
    {
        return "Fov";
    }
};

using LowFov = Fov;
using HighFov = Fov;

REGISTER_EFFECT(LowFov, "40 Fov", 40.0f);
REGISTER_EFFECT(HighFov, "150 Fov", 150.0f);
