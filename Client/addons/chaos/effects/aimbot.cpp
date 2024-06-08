#pragma once

#include "../effect.h"

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
    void SetAIAimbot(bool IsPerfectAimbot)
    {
        const auto aicontrollers = GetTdAIControllers();

        for (size_t i = 0; i < aicontrollers.Num(); i++)
        {
            const auto ai = aicontrollers[i];

            if (!ai) continue;
            if (!ai->myPawn) continue;
            if (ai->myPawn->Health <= 0) continue;

            if (ai->IsA(Classes::ATdAI_Sniper::StaticClass()))
            {
                const auto sniperAi = static_cast<Classes::ATdAI_Sniper*>(ai);
                sniperAi->NormalAimBot->OverriddenImprovementRate = IsPerfectAimbot ? 100.0f : 0.0f;
                sniperAi->SniperAimBot->OverriddenImprovementRate = IsPerfectAimbot ? 100.0f : 0.0f;
            }
            else
            {
                ai->AimBot = IsPerfectAimbot ? ai->PerfectAimBot : ai->RealAimBot;
            }
        }
    }
};

using AiAimbot = Aimbot;

REGISTER_EFFECT(AiAimbot, "AI Have Aimbot");
