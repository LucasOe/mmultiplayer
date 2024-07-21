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

    void Tick(const float deltaTime) override 
    {
        const auto world = Engine::GetWorld(); 
        if (!world)
        {
            return;
        }

        const auto tdgame = static_cast<Classes::ATdSPGame*>(world->Game);
        if (!tdgame)
        {
            return;
        }
        
        tdgame->KillBots();
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
