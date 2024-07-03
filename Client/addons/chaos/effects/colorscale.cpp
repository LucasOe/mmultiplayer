#pragma once

#include "../effect.h"

enum class EColorScaleType
{
    Random,
    Rainbow
};

class ColorScale : public Effect
{
private:
    float Time = 0.0f;
    int Index = 0;
    int PreviousIndex = 0;
    EColorScaleType ColorScaleType;
    Classes::FVector RandomColorScale;

    const std::vector<ImVec4> Colors = {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow
        ImVec4(0.5f, 1.0f, 0.0f, 1.0f),  // Yellow-Green
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green
        ImVec4(0.0f, 1.0f, 0.5f, 1.0f),  // Green-Cyan
        ImVec4(0.0f, 1.0f, 1.0f, 1.0f),  // Cyan
        ImVec4(0.0f, 0.5f, 1.0f, 1.0f),  // Cyan-Blue
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue
        ImVec4(0.5f, 0.0f, 1.0f, 1.0f),  // Blue-Magenta
        ImVec4(1.0f, 0.0f, 1.0f, 1.0f),  // Magenta
        ImVec4(1.0f, 0.0f, 0.5f, 1.0f)   // Magenta-Red
    };

public:
    ColorScale(const std::string& name, EColorScaleType colorScaleType)
    {
        Name = name;
        DisplayName = name;

        ColorScaleType = colorScaleType;
    }

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override 
    {
        Time = 0.0f;
        Index = 0;
        PreviousIndex = 0;

        if (ColorScaleType == EColorScaleType::Random)
        {
            RandomColorScale.X = RandomFloat(0.0f, 1.0f);
            RandomColorScale.Y = RandomFloat(0.0f, 1.0f);
            RandomColorScale.Z = RandomFloat(0.0f, 1.0f);
        }
    }

    void Tick(const float deltaTime) override
    {
        auto controller = Engine::GetPlayerController();

        if (!controller->PlayerCamera)
        {
            return;
        }

        if (ColorScaleType == EColorScaleType::Random)
        {
            controller->PlayerCamera->ColorScale = RandomColorScale;
            return;
        }
        
        Time += deltaTime;
        Time = ImClamp(Time, 0.0f, 1.0f);

        const auto currentColor = ImLerp(Colors[PreviousIndex], Colors[Index], Time);
        controller->PlayerCamera->ColorScale = { currentColor.x, currentColor.y, currentColor.z };

        if (Time >= 1.0f)
        {
            PreviousIndex = Index;
            Index = (Index + 1) % Colors.size();
            Time = 0.0f;
        }
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        auto controller = Engine::GetPlayerController();
        if (controller->PlayerCamera)
        {
            controller->PlayerCamera->ColorScale = { 1.0f, 1.0f, 1.0f };
            return true;
        }

        return false;
    }

    EGroup GetGroup() override
    {
        return EGroup_Camera;
    }

    std::string GetClass() const override
    {
        return "ColorScale";
    }
};

using RandomColorScale = ColorScale;
using RainbowColorScale = ColorScale;

REGISTER_EFFECT(RandomColorScale, "Random Color Scale", EColorScaleType::Random);
REGISTER_EFFECT(RainbowColorScale, "Rainbow Color Scale", EColorScaleType::Rainbow);
