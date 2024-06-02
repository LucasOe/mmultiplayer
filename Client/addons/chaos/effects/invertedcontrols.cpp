#pragma once

#include "../effect.h"

class InvertedControls : public Effect
{
private:
    unsigned long PreviousbInvertMouse = 0;
    unsigned long PreviousbInvertTurn = 0;

public:
    InvertedControls(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Start() override 
    {
        const auto controller = Engine::GetPlayerController();

        if (controller->PlayerInput)
        {
            PreviousbInvertMouse = controller->PlayerInput->bInvertMouse;
            PreviousbInvertTurn = controller->PlayerInput->bInvertTurn;
        }

        // TODO: Find a way if it's possible to invert to controls. W -> S, A -> D etc
    }

    void Tick(const float deltaTime) override
    {
        const auto controller = Engine::GetPlayerController();

        if (controller->PlayerInput)
        {
            controller->PlayerInput->bInvertMouse = PreviousbInvertMouse == TRUE ? FALSE : TRUE;
            controller->PlayerInput->bInvertTurn = PreviousbInvertTurn == TRUE ? FALSE : TRUE;
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto controller = Engine::GetPlayerController();

        if (controller->PlayerInput)
        {
            controller->PlayerInput->bInvertMouse = PreviousbInvertMouse;
            controller->PlayerInput->bInvertTurn = PreviousbInvertTurn;
        }

        return true;
    }

    std::string GetType() const override
    {
        return "InvertedControls";
    }
};

REGISTER_EFFECT(InvertedControls, "Inverted Mouse");
