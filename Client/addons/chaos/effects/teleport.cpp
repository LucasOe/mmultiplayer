#pragma once

#include "../effect.h"

enum class ETeleportType
{
    PlayerToRandomAI,
    AllAIToPlayer,
    LastJumpLocation
};

class Teleport : public Effect
{
private:
    float Time = 0.0f;
    float MinDelay = 2.0f;
    float MaxDelay = 12.0f;
    ETeleportType TeleportType;

public:
    Teleport(const std::string& name, ETeleportType teleportType)
    {
        Name = name;
        DisplayName = name;

        TeleportType = teleportType;
    }

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override 
    {
        Done = false;
        Time = RandomFloat(MinDelay, MaxDelay);
    }

    void Tick(const float deltaTime) override
    {
        if (Done)
        {
            return;
        }

        auto pawn = Engine::GetPlayerPawn();
        auto aicontrollers = GetTdAIControllers();

        if (TeleportType == ETeleportType::PlayerToRandomAI)
        {
            for (size_t i = 0; i < aicontrollers.Num(); i++)
            {
                auto ai = aicontrollers[RandomInt(0, aicontrollers.Num())];

                if (!ai) continue;
                if (!ai->myPawn) continue;
                if (ai->myPawn->Health <= 0) continue;

                pawn->Velocity = { 0.0f, 0.0f, 0.0f };
                pawn->Location = ai->myPawn->Location;

                Done = true;
                return;
            }
        }
        else if (TeleportType == ETeleportType::AllAIToPlayer)
        {
            for (size_t i = 0; i < aicontrollers.Num(); i++)
            {
                auto ai = aicontrollers[i];

                if (!ai) continue;
                if (!ai->myPawn) continue;
                if (ai->myPawn->Health <= 0) continue;
                if (!CanTeleport(ai)) continue;

                ai->bMoveJustFinished = true;
                ai->myPawn->Velocity = { 0.0f, 0.0f, 0.0f };
                ai->myPawn->Location = pawn->Location;

                Done = true;
            }
        }
        else if (TeleportType == ETeleportType::LastJumpLocation)
        {
            if (pawn->LastJumpLocation.X == 0.0f && pawn->LastJumpLocation.Y == 0.0f && pawn->LastJumpLocation.Z == 0.0f)
            {
                return;
            }

            Time -= deltaTime;
            if (Time >= 0.0f || pawn->Health <= 0)
            {
                return;
            }

            Time = RandomFloat(MinDelay, MaxDelay);
            pawn->Location = pawn->LastJumpLocation;
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    EGroup GetGroup() override
    {
        if (TeleportType == ETeleportType::LastJumpLocation)
        {
            return EGroup_Teleport | EGroup_Player;
        }

        return EGroup_Teleport | EGroup_Player | EGroup_AI;
    }

    std::string GetClass() const override
    {
        return "Teleport";
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

        const auto name = ai->GetObjectName();

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

using TeleportToRandomAI = Teleport;
using TeleportAllAIToPlayer = Teleport;
using TeleportLastJumpLocation = Teleport;

REGISTER_EFFECT(TeleportToRandomAI, "Teleport To Random AI", ETeleportType::PlayerToRandomAI);
REGISTER_EFFECT(TeleportAllAIToPlayer, "Teleport All AI To Player", ETeleportType::AllAIToPlayer);
REGISTER_EFFECT(TeleportLastJumpLocation, "Teleport To LastJumpLocation Randomly", ETeleportType::LastJumpLocation);
