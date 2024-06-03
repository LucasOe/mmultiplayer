#pragma once

#include "../effect.h"
#include <d3dx9tex.h>
#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3dx9")

struct Image 
{
    int Width = 0;
    int Height = 0;
    PDIRECT3DTEXTURE9 Texture = NULL;
    std::string Filename;
    bool HasLoadedFromFile = false;
};

class DvdScreenSaver : public Effect
{
private:
    ImVec2 Position;
    ImVec2 Speed;
    Image Image;

public:
    DvdScreenSaver(const std::string& name, const std::string& filename)
    {
        Name = name;
        DisplayName = name;
        DurationType = EffectDuration::Long;

        Speed = ImVec2(128.0f, 128.0f);
        Image.Filename = filename;
    }

    void Start() override 
    {
        Position.x = static_cast<float>(RandomInt(0, 256));
        Position.y = static_cast<float>(RandomInt(0, 256));
    }

    void Tick(const float deltaTime) override 
    {
        const auto& io = ImGui::GetIO();

        // FIXME: If the player respawns and the image is about to hit a boundary, it could go out of bounds or get stuck  
        Position += Speed * deltaTime;

        if (Position.x <= 0.0f || Position.x + Image.Width >= io.DisplaySize.x)
        {
            Speed.x = -Speed.x;
        }

        if (Position.y <= 0.0f || Position.y + Image.Height >= io.DisplaySize.y)
        {
            Speed.y = -Speed.y;
        }
    }

    void Render(IDirect3DDevice9* device) override
    {
        if (Image.HasLoadedFromFile == false)
        {
            IDirect3DTexture9* texture;
            HRESULT hr = D3DXCreateTextureFromFileA(device, Image.Filename.c_str(), &texture);
            if (hr != S_OK)
            {
                printf("Chaos DvdScreenSaver: Failed to create texture!\n");
                return;
            }

            D3DSURFACE_DESC image_desc;
            texture->GetLevelDesc(0, &image_desc);
            Image.Texture = texture;
            Image.Width = image_desc.Width;
            Image.Height = image_desc.Height;
            Image.HasLoadedFromFile = true;

            printf("Chaos DvdScreenSaver: Image loaded successfully\n");
        }

        auto window = ImGui::BeginRawScene("##DvdScreenSaver");

        ImGui::SetWindowPos(window, Position, ImGuiCond_Always);
        ImGui::Image((void*)Image.Texture, ImVec2(static_cast<float>(Image.Width), static_cast<float>(Image.Height)));
        ImGui::EndRawScene();
    }

    bool Shutdown() override 
    {
        return true;
    }

    std::string GetType() const override
    {
        return "DvdScreenSaver";
    }
};

using DvdScreenSaverEffect = DvdScreenSaver;

//REGISTER_EFFECT(DvdScreenSaverEffect, "Dvd ScreenSaver", "dvd.png");
