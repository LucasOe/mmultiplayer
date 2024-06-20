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

    std::string GetType() const override
    {
        return "InvertedControls";
    }

    EGroup GetGroup() override
    {
        return EGroup_Camera;
    }

    EGroup GetIncompatibleGroup() override
    {
        return EGroup_None;
    }
};

REGISTER_EFFECT(InvertedControls, "Inverted Mouse");
