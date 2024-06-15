#pragma once

#include "../effect.h"

class Framerate : public Effect
{
private:
    bool bSmoothFrameRate = true;
    bool PreviousbSmoothFrameRate = true;
    float FrameRateToApply = 62.0f;
    float PreviousMaxFrameRate = 62.0f;

public:
    Framerate(const std::string& name, float framerate, bool smoothFramerate)
    {
        Name = name;
        DisplayName = name;

        FrameRateToApply = framerate;
        bSmoothFrameRate = smoothFramerate;
    }

    void Start() override 
    {
        auto engine = Engine::GetEngine();
        if (!engine)
        {
            return;
        }

        PreviousMaxFrameRate = engine->MaxSmoothedFrameRate;
        PreviousbSmoothFrameRate = engine->bSmoothFrameRate;
    }

    void Tick(const float deltaTime) override
    {
        auto engine = Engine::GetEngine();
        if (!engine)
        {
            return;
        }

        engine->MaxSmoothedFrameRate = FrameRateToApply;
        engine->bSmoothFrameRate = bSmoothFrameRate;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        auto engine = Engine::GetEngine();
        if (!engine)
        {
            return false;
        }

        engine->MaxSmoothedFrameRate = PreviousMaxFrameRate;
        engine->bSmoothFrameRate = PreviousbSmoothFrameRate;

        return true;
    }

    std::string GetType() const override 
    { 
        return "Framerate"; 
    }
};

using ConsoleFramerate = Framerate;
using UncappedFramerate = Framerate;

REGISTER_EFFECT(ConsoleFramerate, "Console Experience", 30.0f, TRUE);
REGISTER_EFFECT(UncappedFramerate, "No Framerate Cap", 0.0f, FALSE);
