#pragma once

#include "../effect.h"

class GoToMainMenu : public Effect
{
private:
    float TimeLeft = 5.0f;

public:
    GoToMainMenu(const std::string& name)
    {
        Name = name;
        DurationType = EDuration::Short;
    }

    void Start() override
    {
        Done = false;
        TimeLeft = 5.0f;
    }

    void Tick(const float deltaTime) override 
    {
        if (Done)
        {
            return;
        }

        char buffer[0x40];
        sprintf_s(buffer, sizeof(buffer), "Go To MainMenu In... %.1f", TimeLeft >= 0.0f ? TimeLeft : 0.0f);
        DisplayName = buffer;

        TimeLeft -= deltaTime;

        if (TimeLeft >= 0.0f)
        {
            return;
        }

        if (Engine::GetPlayerController()->IsInMainMenu())
        {
            DisplayName = "Go To MainMenu";
            Done = true;
        }
        else
        {
            const auto gameInfo = static_cast<Classes::ATdGameInfo*>(Engine::GetWorld()->Game);

            if (!gameInfo->TdGameData)
            {
                return;
            }

            gameInfo->TdGameData->QuitToMainMenu();
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "GoToMainMenu";
    }

    EGroup GetGroup() override
    {
        return EGroup_None;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_None;
    }
};

REGISTER_EFFECT(GoToMainMenu, "Go To MainMenu");
