#pragma once

#include "../effect.h"

class KillBots : public Effect
{
public:
    KillBots(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override {}

    void Tick(const float deltaTime) override 
    {
        const auto world = Engine::GetWorld();
        const auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);

        // Null check here since I'm not 100% sure if it could be null at some point
        if (!tdgame)
        {
            return;
        }
        
        tdgame->KillBots();
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "KillBots";
    }
};

REGISTER_EFFECT(KillBots, "KillBots");
