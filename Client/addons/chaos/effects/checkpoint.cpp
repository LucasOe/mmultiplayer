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
        auto world = Engine::GetWorld();
        auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);

        if (!tdgame)
        {
            return;
        }

        tdgame->bShouldSaveCheckpointProgress = false;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        auto world = Engine::GetWorld();
        auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);

        if (!tdgame)
        {
            return false;
        }

        tdgame->bShouldSaveCheckpointProgress = true;
        return true;
    }

    std::string GetType() const override
    {
        return "Checkpoint";
    }
};

REGISTER_EFFECT(Checkpoint, "Disable Saving Checkpoint Progress");
