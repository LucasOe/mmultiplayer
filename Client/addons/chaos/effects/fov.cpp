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
            SetLockedFov(true);
            return;
        }

        if (pawn->MyWeapon->GetObjectName() != "TdWeapon_Sniper_BarretM95")
        {
            SetLockedFov(true);
            return;
        }

        const auto sniper = static_cast<Classes::ATdWeapon_Sniper_BarretM95*>(pawn->MyWeapon);
        SetLockedFov(!(sniper->bZoomed || sniper->bIsZooming));
    }

    bool Shutdown() override 
    {
        const auto controller = Engine::GetPlayerController();

        if (!controller->PlayerCamera)
        {
            return false;
        }

        SetLockedFov(false);
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_None;
    }

    std::string GetClass() const override
    {
        return "Fov";
    }

private:
    void SetLockedFov(bool lockFov)
    {
        auto controller = Engine::GetPlayerController();
        controller->PlayerCamera->LockedFOV = lockFov ? NewFov : 0.0f;
        controller->PlayerCamera->bLockedFOV = lockFov;
    }
};

using LowFov = Fov;
using HighFov = Fov;

REGISTER_EFFECT(LowFov, "60 Fov", 60.0f);
REGISTER_EFFECT(HighFov, "150 Fov", 150.0f);
