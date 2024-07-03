#pragma once

#include "../effect.h"

class InvertedControls : public Effect
{
private:
    bool PreviousbInvertMouse = 0;
    bool PreviousbInvertTurn = 0;

public:
    InvertedControls(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override 
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
        auto controller = Engine::GetPlayerController();

        if (controller->PlayerInput)
        {
            controller->PlayerInput->bInvertMouse = !PreviousbInvertMouse;
            controller->PlayerInput->bInvertTurn = !PreviousbInvertTurn;
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        auto controller = Engine::GetPlayerController();

        if (!controller->PlayerInput)
        {
            return false;
        }

        controller->PlayerInput->bInvertMouse = PreviousbInvertMouse;
        controller->PlayerInput->bInvertTurn = PreviousbInvertTurn;

        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Mouse;
    }

    std::string GetClass() const override
    {
        return "InvertedControls";
    }
};

REGISTER_EFFECT(InvertedControls, "Inverted Mouse");
