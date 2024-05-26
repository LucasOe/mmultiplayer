#pragma once

#include "../effect.h"

class DvdScreenSaver : public Effect
{
public:
    DvdScreenSaver(const std::string& name)
    {
        Name = name;
    }

    void OnStart() override 
    {
        // TODO: Load image
    }

    void OnTick(const float deltaTime) override 
    {
        const auto window = ImGui::BeginRawScene("##DvdScreenSaver");
        const auto& io = ImGui::GetIO();

        // TODO: Move Image

        ImGui::EndRawScene();
    }

    void OnEnd() override 
    {
        // Free image
    }
};

//REGISTER_EFFECT(DvdScreenSaver, "Dvd ScreenSaver");
