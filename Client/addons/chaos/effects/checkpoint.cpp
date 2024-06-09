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

    void Start() override {}

    void Tick(const float deltaTime) override
    {
        const auto world = Engine::GetWorld();
        const auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);

        if (!tdgame)
        {
            return;
        }

        tdgame->bShouldSaveCheckpointProgress = FALSE;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto world = Engine::GetWorld();
        const auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);

        if (!tdgame)
        {
            return false;
        }

        tdgame->bShouldSaveCheckpointProgress = TRUE;
        return true;
    }

    std::string GetType() const override
    {
        return "Checkpoint";
    }
};

REGISTER_EFFECT(Checkpoint, "Disable Saving Checkpoint Progress");
