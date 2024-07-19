#pragma once

#include "../effect.h"
#include <map>

struct Weapon
{
    float WeaponRange;
    float Spread;
    float KickbackAmount;
    float RecoilAmount;

    Weapon(float weaponRange = 12000.0f, float spread = 0.05f, float kickbackAmount = 20.0f, float recoilAmount = 0.8f)
    {
        WeaponRange = weaponRange;
        Spread = spread;
        KickbackAmount = kickbackAmount;
        RecoilAmount = recoilAmount;
    }
};

class Aimbot : public Effect
{
private:
    bool PlayerHasAimbot = false;

public:
    Aimbot(const std::string& name, bool playerHasAimbot)
    {
        Name = name;
        DisplayName = name;

        PlayerHasAimbot = playerHasAimbot;
    }

    void Tick(const float deltaTime) override
    {
        if (PlayerHasAimbot)
        {
            SetPlayerAimbot(true);
            return;
        }
        
        SetAIAimbot(true);
    }

    bool Shutdown() override
    {
        if (PlayerHasAimbot)
        {
            SetPlayerAimbot(false);
            return true;
        }

        SetAIAimbot(false);
        return true;
    }

    EGroup GetGroup() override
    {
        if (PlayerHasAimbot)
        {
            return EGroup_Camera | EGroup_Weapon | EGroup_Player;
        }

        return EGroup_Camera | EGroup_Weapon | EGroup_AI;
    }

    std::string GetClass() const override 
    {
        return "Aimbot";
    }

private:
    Weapon GetDefaults(Classes::ATdWeapon* weapon)
    {
        static const std::map<std::string, Weapon> weaponDefaults = {
            {"TdWeapon_Pistol_Colt1911",        { 4500.0f, 0.03f, 10.0f, 2.0f }},
            {"TdWeapon_Pistol_BerettaM93R",     { 4500.0f, 0.03f, 10.0f, 0.5f }},
            {"TdWeapon_Pistol_Glock18c",        { 4500.0f, 0.03f, 10.0f, 2.0f }},
            {"TdWeapon_SMG_SteyrTMP",           { 3000.0f, 0.008f, 10.0f, 0.6f }},
            {"TdWeapon_AssaultRifle_FNSCARL",   { 9000.0f, 0.02f, 20.0f, 0.8f }},
            {"TdWeapon_AssaultRifle_HKG36",     { 9000.0f, 0.03f, 20.0f, 0.8f }},
            {"TdWeapon_AssaultRifle_MP5K",      { 8000.0f, 0.04f, 20.0f, 0.8f }},
            {"TdWeapon_Shotgun_Neostead",       { 6000.0f, 0.1f, 50.0f, 3.0f }},
            {"TdWeapon_Shotgun_Remington870",   { 6000.0f, 0.12f, 50.0f, 3.0f }},
            {"TdWeapon_Machinegun_FNMinimi",    { 20000.0f, 0.04f, 50.0f, 1.0f }},
            {"TdWeapon_Sniper_BarretM95",       { 20000.0f, 0.001f, 10.0f, 0.8f }}
        };

        auto it = weaponDefaults.find(weapon->GetObjectName());
        if (it != weaponDefaults.end())
        {
            return it->second;
        }

        return Weapon();
    }

    void SetWeapon(Classes::ATdWeapon* weapon, bool IsAimbotActive)
    {
        if (!weapon)
        {
            return;
        }

        const auto name = weapon->GetObjectName();

        if (name == "TdWeapon_Sniper_BarretM95")
        {
            auto sniper = static_cast<Classes::ATdWeapon_Sniper_BarretM95*>(weapon);
            sniper->AdditionalUnzoomedSpread = IsAimbotActive ? 0.0f : 0.5f;
        }
        else if (name == "TdWeapon_Pistol_TaserContent" || name == "TdWeapon_Pistol_Taser")
        {
            return;
        }

        weapon->Spread[0] = IsAimbotActive ? 0.0f : GetDefaults(weapon).Spread;
        weapon->RecoilAmount = IsAimbotActive ? 0.0f : GetDefaults(weapon).RecoilAmount;
        weapon->KickbackAmount = IsAimbotActive ? 0.0f : GetDefaults(weapon).KickbackAmount;
        weapon->WeaponRange = weapon->CachedMaxRange = IsAimbotActive ? 100000.0f : GetDefaults(weapon).WeaponRange;
    }

    void SetAIAimbot(bool IsAimbotActive)
    {
        auto aicontrollers = GetTdAIControllers();

        for (size_t i = 0; i < aicontrollers.Num(); i++)
        {
            auto ai = aicontrollers[i];

            if (!ai) continue;
            if (!ai->myPawn) continue;
            if (ai->myPawn->Health <= 0) continue;

            if (ai->GetObjectName() == "TdAI_Sniper")
            {
                auto sniperAi = static_cast<Classes::ATdAI_Sniper*>(ai);
                sniperAi->NormalAimBot->OverriddenImprovementRate = IsAimbotActive ? 100.0f : 0.0f;
                sniperAi->SniperAimBot->OverriddenImprovementRate = IsAimbotActive ? 100.0f : 0.0f;
            }
            else
            {
                ai->AimBot = IsAimbotActive ? ai->PerfectAimBot : ai->RealAimBot;
            }

            SetWeapon(ai->myPawn->MyWeapon, IsAimbotActive);
        }
    }

    void SetPlayerAimbot(bool IsAimbotActive)
    {
        auto controller = Engine::GetPlayerController();
        controller->InfiniteAmmo = IsAimbotActive;

        auto pawn = Engine::GetPlayerPawn();
        SetWeapon(pawn->MyWeapon, IsAimbotActive);

        if (!pawn->MyWeapon)
        {
            return;
        }

        for (size_t i = 0; i < controller->LocalEnemies.Num(); i++)
        {
            auto enemy = controller->LocalEnemies[i].Enemy;
            if (!enemy)
            {
                continue;
            }

            if (!controller->LocalEnemies[i].bVisible)
            {
                continue;
            }

            auto location = enemy->Location;
            if (enemy->Mesh3p)
            {
                location = enemy->Mesh3p->GetBoneLocation("Head", 0);
                location.Z -= 64.0f;
            }

            Classes::FVector direction = { 
                location.X - pawn->Location.X,
                location.Y - pawn->Location.Y,
                location.Z - pawn->Location.Z
            };

            const int yawDegrees = static_cast<int>(atan2(direction.Y, direction.X) * 65536.0f / CONST_Tau);

            const float distanceXY = sqrt(direction.X * direction.X + direction.Y * direction.Y);
            const float pitchRadians = atan2(direction.Z, distanceXY);

            int pitchDegrees = static_cast<int>(pitchRadians * 32768.0f / CONST_Pi); // Between 0 and 16383
            if (pitchRadians < 0.0f)
            {
                pitchDegrees += 65536; // Between 49152 and 65535
            }

            controller->Rotation.Pitch = pitchDegrees;
            controller->Rotation.Yaw = yawDegrees;
            break;
        }
    }
};

using AiAimbot = Aimbot;
using PlayerAimbot = Aimbot;

REGISTER_EFFECT(AiAimbot, "AI Have Aimbot", false);
REGISTER_EFFECT(PlayerAimbot, "You Have Aimbot", true);
