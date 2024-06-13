#pragma once

#include "../effect.h"
#include <map>

struct Weapon
{
    float WeaponRange;
    float Spread;

    Weapon(float weaponRange = 8000.0f, float spread = 0.05f)
    {
        WeaponRange = weaponRange;
        Spread = spread;
    }
};

class Aimbot : public Effect
{
public:
    Aimbot(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        SetAIAimbot(true);
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        SetAIAimbot(false);
        return true;
    }

    std::string GetType() const override
    {
        return "Aimbot";
    }

private:
    Weapon GetDefaults(Classes::ATdWeapon* weapon)
    {
        static const std::map<std::string, Weapon> weaponDefaults = {
            {"TdWeapon_Pistol_Colt1911",        { 4500.0f, 0.03f }},
            {"TdWeapon_Pistol_BerettaM93R",     { 4500.0f, 0.03f }},
            {"TdWeapon_Pistol_Glock18c",        { 4500.0f, 0.03f }},
            {"TdWeapon_SMG_SteyrTMP",           { 3000.0f, 0.008f }},
            {"TdWeapon_AssaultRifle_FNSCARL",   { 9000.0f, 0.02f }},
            {"TdWeapon_AssaultRifle_HKG36",     { 9000.0f, 0.03f }},
            {"TdWeapon_AssaultRifle_MP5K",      { 8000.0f, 0.04f }},
            {"TdWeapon_Shotgun_Neostead",       { 6000.0f, 0.1f }},
            {"TdWeapon_Shotgun_Remington870",   { 6000.0f, 0.12f }},
            {"TdWeapon_Machinegun_FNMinimi",    { 20000.0f, 0.04f }},
            {"TdWeapon_Sniper_BarretM95",       { 20000.0f, 0.001f }}
        };

        auto it = weaponDefaults.find(GetObjectName(weapon));
        if (it != weaponDefaults.end())
        {
            return it->second;
        }

        return Weapon();
    }

    void SetWeapon(Classes::ATdWeapon* weapon, bool IsPerfectAimbot)
    {
        if (!weapon)
        {
            return;
        }

        const std::string name = GetObjectName(weapon);

        if (name == "TdWeapon_Sniper_BarretM95")
        {
            const auto sniper = static_cast<Classes::ATdWeapon_Sniper_BarretM95*>(weapon);
            sniper->AdditionalUnzoomedSpread = IsPerfectAimbot ? 0.0f : 0.5f;
        }
        else if (name == "TdWeapon_Pistol_TaserContent" || name == "TdWeapon_Pistol_Taser")
        {
            return;
        }

        // Turns off spread and increases the weapon range to 1km
        weapon->Spread[0] = IsPerfectAimbot ? 0.0f : GetDefaults(weapon).Spread;
        weapon->WeaponRange = weapon->CachedMaxRange = IsPerfectAimbot ? 100000.0f : GetDefaults(weapon).WeaponRange;
    }

    void SetAIAimbot(bool IsPerfectAimbot)
    {
        const auto aicontrollers = GetTdAIControllers();

        for (size_t i = 0; i < aicontrollers.Num(); i++)
        {
            const auto ai = aicontrollers[i];

            if (!ai) continue;
            if (!ai->myPawn) continue;
            if (ai->myPawn->Health <= 0) continue;

            if (GetObjectName(ai) == "TdAI_Sniper")
            {
                const auto sniperAi = static_cast<Classes::ATdAI_Sniper*>(ai);
                sniperAi->NormalAimBot->OverriddenImprovementRate = IsPerfectAimbot ? 100.0f : 0.0f;
                sniperAi->SniperAimBot->OverriddenImprovementRate = IsPerfectAimbot ? 100.0f : 0.0f;
            }
            else
            {
                ai->AimBot = IsPerfectAimbot ? ai->PerfectAimBot : ai->RealAimBot;
            }

            SetWeapon(ai->myPawn->MyWeapon, IsPerfectAimbot);
        }
    }
};

using AiAimbot = Aimbot;

REGISTER_EFFECT(AiAimbot, "AI Have Aimbot");
