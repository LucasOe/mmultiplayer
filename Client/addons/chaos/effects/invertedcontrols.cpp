#pragma once

#include "../effect.h"

class InvertedControls : public Effect
{
private:
    bool PreviousbInvertMouse = false;

public:
    InvertedControls(const std::string& name)
    {
        Name = name;
        DisplayName = name;
    }

    void Initialize() override 
    {
        const auto controller = Engine::GetPlayerController();

        if (controller->PlayerInput)
        {
            PreviousbInvertMouse = controller->PlayerInput->bInvertMouse;
        }
    }

    void Tick(const float deltaTime) override
    {
        auto controller = Engine::GetPlayerController();

        if (controller->PlayerInput)
        {
            controller->PlayerInput->bInvertMouse = !PreviousbInvertMouse;
        }
    }

    bool Shutdown() override
    {
        auto controller = Engine::GetPlayerController();

        if (!controller->PlayerInput)
        {
            return false;
        }

        controller->PlayerInput->bInvertMouse = PreviousbInvertMouse;

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
