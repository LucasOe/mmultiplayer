#pragma once

#include "../effect.h"

class Checkpoint : public Effect
{
public:
    Checkpoint(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    bool CanActivate() override
    {
        const auto world = Engine::GetWorld();
        if (!world)
        {
            return false;
        }

        const auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);
        if (!tdgame)
        {
            return false;
        }

        return tdgame->bShouldSaveCheckpointProgress;
    }

    void Tick(const float deltaTime) override
    {
        auto world = Engine::GetWorld();
        if (!world)
        {
            return;
        }

        auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);
        if (!tdgame)
        {
            return;
        }

        tdgame->bShouldSaveCheckpointProgress = false;
    }

    bool Shutdown() override
    {
        auto world = Engine::GetWorld();
        if (!world)
        {
            return false;
        }

        auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);
        if (!tdgame)
        {
            return false;
        }

        tdgame->bShouldSaveCheckpointProgress = true;
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Disable | EGroup_Checkpoint;
    }

    std::string GetClass() const override
    {
        return "Checkpoint";
    }
};

REGISTER_EFFECT(Checkpoint, "Disable Saving Checkpoint Progress");
