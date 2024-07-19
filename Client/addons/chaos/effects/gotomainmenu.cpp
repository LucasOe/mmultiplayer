#pragma once

#include "../effect.h"

class GoToMainMenu : public Effect
{
public:
    GoToMainMenu(const std::string& name)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Short;
    }

    void Initialize() override 
    {
        Done = false;
    }

    void Tick(const float deltaTime) override 
    {
        if (Done)
        {
            return;
        }

        const auto gameInfo = static_cast<Classes::ATdGameInfo*>(Engine::GetWorld()->Game);
        if (!gameInfo->TdGameData)
        {
            return;
        }

        gameInfo->TdGameData->QuitToMainMenu();
        Done = true;
    }

    EGroup GetGroup() override
    {
        return EGroup_None;
    }

    std::string GetClass() const override
    {
        return "GoToMainMenu";
    }
};

REGISTER_EFFECT(GoToMainMenu, "Go To MainMenu");
