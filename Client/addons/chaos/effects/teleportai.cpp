#pragma once

#include "../effect.h"

enum class ETeleportType
{
    PlayerToRandomAI,
    AllAIToPlayer
};

class TeleportAI : public Effect
{
private:
    ETeleportType TeleportType;

public:
    TeleportAI(const std::string& name, ETeleportType teleportType)
    {
        Name = name;
        DisplayName = name;

        TeleportType = teleportType;
    }

    void Start() override
    {
        IsDone = false;
    }

    void Tick(const float deltaTime) override
    {
        if (IsDone)
        {
            return;
        }

        const auto pawn = Engine::GetPlayerPawn();
        const auto aicontrollers = GetTdAIControllers();

        if (TeleportType == ETeleportType::PlayerToRandomAI)
        {
            for (size_t i = 0; i < aicontrollers.Num(); i++)
            {
                const auto ai = aicontrollers[RandomInt(0, aicontrollers.Num())];

                if (!ai) continue;
                if (!ai->myPawn) continue;
                if (ai->myPawn->Health <= 0) continue;

                pawn->Velocity = { 0.0f, 0.0f, 0.0f };
                pawn->Location = ai->myPawn->Location;

                IsDone = true;
                return;
            }
        }
        else if (TeleportType == ETeleportType::AllAIToPlayer)
        {
            for (size_t i = 0; i < aicontrollers.Num(); i++)
            {
                const auto ai = aicontrollers[i];

                if (!ai) continue;
                if (!ai->myPawn) continue;
                if (ai->myPawn->Health <= 0) continue;
                if (!CanTeleport(ai)) continue;

                ai->bMoveJustFinished = TRUE;
                ai->myPawn->Velocity = { 0.0f, 0.0f, 0.0f };
                ai->myPawn->Location = pawn->Location;

                IsDone = true;
            }
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "AI";
    }

private:
    bool CanTeleport(Classes::ATdAIController* ai)
    {
        static const std::vector<std::string> validAINames = {
            "TdAI_PatrolCop",
            "TdAI_Assault",
            "TdAI_Pursuit",
            "TdAI_Sniper",
            "TdAI_Support",
            "TdAI_Riot",
        };

        const std::string name = GetObjectName(ai);

        for (const auto& validName : validAINames)
        {
            if (name == validName)
            {
                return true;
            }
        }

        return false;
    }
};

using TeleportToAI = TeleportAI;
using TeleportAllAIToPlayer = TeleportAI;

REGISTER_EFFECT(TeleportToAI, "Teleport To Random AI", ETeleportType::PlayerToRandomAI);
REGISTER_EFFECT(TeleportAllAIToPlayer, "Teleport All AI To Player", ETeleportType::AllAIToPlayer);
