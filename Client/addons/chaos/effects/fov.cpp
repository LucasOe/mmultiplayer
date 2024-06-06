#pragma once

#include "../effect.h"

class Fov : public Effect
{
private:
    float NewFov = 100.0f;

public:
    Fov(const std::string& name, float newFov)
    {
        Name = name;
        DisplayName = name;

        NewFov = newFov;
    }

    void Start() override {}

    void Tick(const float deltaTime) override 
    {
        const auto pawn = Engine::GetPlayerPawn();
        const auto controller = Engine::GetPlayerController();

        if (!controller->PlayerCamera)
        {
            return;
        }

        if (!pawn->MyWeapon)
        {
            SetFov(NewFov, TRUE);
            return;
        }

        if (!pawn->MyWeapon->IsA(Classes::ATdWeapon_Sniper_BarretM95::StaticClass()))
        {
            SetFov(NewFov, TRUE);
            return;
        }

        const auto sniper = static_cast<Classes::ATdWeapon_Sniper_BarretM95*>(pawn->MyWeapon);

        if (sniper->bZoomed || sniper->bIsZooming)
        {
            SetFov(NewFov, FALSE);
            return;
        }

        SetFov(NewFov, TRUE);
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override 
    {
        const auto controller = Engine::GetPlayerController();

        if (!controller->PlayerCamera)
        {
            return false;
        }

        SetFov(NewFov, FALSE);
        return true;
    }

    std::string GetType() const override
    {
        return "Fov";
    }

private:
    void SetFov(float newFov, unsigned long lockFov)
    {
        const auto controller = Engine::GetPlayerController();
        controller->PlayerCamera->LockedFOV = newFov;
        controller->PlayerCamera->bLockedFOV = lockFov;
    }
};

using LowFov = Fov;
using HighFov = Fov;

REGISTER_EFFECT(LowFov, "60 Fov", 60.0f);
REGISTER_EFFECT(HighFov, "150 Fov", 150.0f);
