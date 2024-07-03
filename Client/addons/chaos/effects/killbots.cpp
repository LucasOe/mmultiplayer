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

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override {}

    void Tick(const float deltaTime) override 
    {
        const auto world = Engine::GetWorld();
        const auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);

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

    EGroup GetGroup() override
    {
        return EGroup_None;
    }

    std::string GetClass() const override
    {
        return "KillBots";
    }
};

REGISTER_EFFECT(KillBots, "KillBots");
