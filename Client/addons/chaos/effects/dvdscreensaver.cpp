#pragma once

#include "../effect.h"

class DvdScreenSaver : public Effect
{
public:
    DvdScreenSaver(const std::string& name)
    {
        Name = name;

        // TODO: Load image
    }

    void Start() override {}

    void Tick(const float deltaTime) override 
    {
        auto window = ImGui::BeginRawScene("##DvdScreenSaver");
        auto& io = ImGui::GetIO();

        // TODO: Move Image

        ImGui::EndRawScene();
    }

    bool Shutdown() override 
    {
        return true;
    }

    std::string GetType() override
    {
        return "DvdScreenSaver";
    }
};

//REGISTER_EFFECT(DvdScreenSaver, "Dvd ScreenSaver");
