#pragma once

#include "../effect.h"

class Framerate : public Effect
{
private:
    unsigned long bSmoothFrameRate = 1;
    unsigned long PreviousbSmoothFrameRate = 1;
    float FrameRateToApply = 0.0f;
    float PreviousMaxFrameRate = 0.0f;

public:
    Framerate(const std::string& name, float framerate, unsigned long smoothFramerate)
    {
        Name = name;
        DisplayName = name;

        FrameRateToApply = framerate;
        bSmoothFrameRate = smoothFramerate;
    }

    void Start() override 
    {
        const auto engine = Engine::GetEngine();
        PreviousMaxFrameRate = engine->MaxSmoothedFrameRate;
        PreviousbSmoothFrameRate = engine->bSmoothFrameRate;
    }

    void Tick(const float deltaTime) override
    {
        const auto engine = Engine::GetEngine();
        engine->MaxSmoothedFrameRate = FrameRateToApply;
        engine->bSmoothFrameRate = bSmoothFrameRate;
    }

    void Render(IDirect3DDevice9* device) override {}

    bool Shutdown() override
    {
        const auto engine = Engine::GetEngine();
        engine->MaxSmoothedFrameRate = PreviousMaxFrameRate;
        engine->bSmoothFrameRate = PreviousbSmoothFrameRate;

        return engine->MaxSmoothedFrameRate == PreviousMaxFrameRate && engine->bSmoothFrameRate == PreviousbSmoothFrameRate;
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
