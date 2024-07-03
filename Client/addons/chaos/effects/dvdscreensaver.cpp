#pragma once

#include "../effect.h"

class DvdScreenSaver : public Effect
{
private:
    ImVec2 Position;
    ImVec2 Speed;
    ImVec2 Size;
    float ScaleFactor;
    ImVec2 DefaultDisplaySize;

public:
    DvdScreenSaver(const std::string& name)
    {
        Name = name;
        DisplayName = name;

        Speed = ImVec2(128.0f, 128.0f);
        ScaleFactor = 0.3f;
    }

    bool CanActivate() override
    {
        return true;
    }

    void Initialize() override 
    {
        const auto& io = ImGui::GetIO();
        DefaultDisplaySize = io.DisplaySize;
    }

    void Tick(const float deltaTime) override 
    {
        const auto& io = ImGui::GetIO();

        Position += Speed * deltaTime;

        if (Position.x <= 0.0f || Position.x + Size.x >= io.DisplaySize.x)
        {
            Speed.x = -Speed.x;
        }

        if (Position.y <= 0.0f || Position.y + Size.y >= io.DisplaySize.y)
        {
            Speed.y = -Speed.y;
        }
    }

    void Render(IDirect3DDevice9* device) override
    {
        const auto window = ImGui::BeginRawScene("##DvdScreenSaver");
        const auto& io = ImGui::GetIO();

        if (DefaultDisplaySize != io.DisplaySize)
        {
            Position = ImVec2(0.0f, 0.0f);
            Speed = ImVec2(128.0f, 128.0f);
            DefaultDisplaySize = io.DisplaySize;
        }

        Size.x = io.DisplaySize.x * ScaleFactor;
        Size.y = io.DisplaySize.y * ScaleFactor;

        const ImVec2 dvdTopLeft = Position;
        const ImVec2 dvdBottomRight = { Position.x + Size.x, Position.y + Size.y };

        window->DrawList->AddRectFilled({ 0, 0 }, { dvdTopLeft.x + Size.x, dvdTopLeft.y }, ImColor(0, 0, 0));
        window->DrawList->AddRectFilled({ dvdBottomRight.x, 0 }, { io.DisplaySize.x, dvdTopLeft.y + Size.y }, ImColor(0, 0, 0));
        window->DrawList->AddRectFilled({ dvdBottomRight.x - Size.x, dvdBottomRight.y }, io.DisplaySize, ImColor(0, 0, 0));
        window->DrawList->AddRectFilled({ 0, dvdBottomRight.y - Size.y }, { dvdTopLeft.x, io.DisplaySize.y }, ImColor(0, 0, 0));

        ImGui::EndRawScene();
    }

    bool Shutdown() override 
    {
        return true;
    }

    EGroup GetGroup() override
    {
        return EGroup_Hud;
    }

    std::string GetClass() const override
    {
        return "DvdScreenSaver";
    }
};

REGISTER_EFFECT(DvdScreenSaver, "Dvd ScreenSaver");
