#pragma once

#include "../effect.h"

class GoToMainMenu : public Effect
{
private:
    float TimeLeft = 5.0f;
    bool IsActuallyDone = false;

public:
    GoToMainMenu(const std::string& name)
    {
        Name = name;
        DurationType = EDuration::Short;
    }

    void Start() override
    {
        IsDone = true;
        IsActuallyDone = false;
        TimeLeft = 5.0f;
    }

    void Tick(const float deltaTime) override 
    {
        if (IsActuallyDone)
        {
            DisplayName = "Go To MainMenu";
            return;
        }

        char buffer[0xFF];
        sprintf_s(buffer, sizeof(buffer), "Go To MainMenu In... %.1f", TimeLeft >= 0.0f ? TimeLeft : 0.0f);
        DisplayName = buffer;

        TimeLeft -= deltaTime;

        if (TimeLeft >= 0.0f)
        {
            return;
        }

        if (Engine::GetPlayerController()->IsInMainMenu())
        {
            IsActuallyDone = true;
        }

        const auto gameInfo = static_cast<Classes::ATdGameInfo*>(Engine::GetWorld()->Game);
        gameInfo->TdGameData->QuitToMainMenu();
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
};

// Removed this effect for now
// REGISTER_EFFECT(GoToMainMenu, "Go To MainMenu");
