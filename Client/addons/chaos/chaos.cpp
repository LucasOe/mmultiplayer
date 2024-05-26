#include "chaos.h"

#include "../chaos/effect.h"
#include "../../menu.h"
#include "../../settings.h"
#include "../../util.h"

static bool Enabled = false;

static float MaxTime = 20.0f;
static float TimerHeight = 18.0f;
static float TimerOffsetTop = 0.0f;
static ImVec4 BackgroundColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 TimerColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);

static float TimeSeconds;
std::vector<Effect*> ActiveEffects;

static void ChaosTab() 
{
    ImGui::Checkbox("Enabled##Chaos-Enabled", &Enabled);

    if (ImGui::SliderFloat("Time##Chaos-MaxTime", &MaxTime, 10.0f, 60.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
    {
        TimeSeconds = 0.0f;
    }

    const auto& io = ImGui::GetIO();

    ImGui::SliderFloat("Timer Height##Chaos-Timer-Height", &TimerHeight, 1.0f, 30.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::ColorEdit4("Background Color##Chaos-Timer-BackgroundColor", &BackgroundColor.x);
    ImGui::ColorEdit4("Timer Color##Chaos-Timer-Color", &TimerColor.x);
    ImGui::Separator(2.5f);
    ImGui::Text("TimeSeconds: %f", TimeSeconds);
}

static void OnRender(IDirect3DDevice9*) 
{
    if (!Enabled) 
    {
        return;
    }

    const auto window = ImGui::BeginRawScene("##Chaos-Timer-Render");
    const auto& io = ImGui::GetIO();

    window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x, TimerHeight), ImColor(BackgroundColor));
    window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x * TimeSeconds / MaxTime, TimerHeight), ImColor(TimerColor));

    ImGui::EndRawScene();

    // Temp
    {
        ImGui::Begin("Active Effects");

        for (auto effect : ActiveEffects)
        {
            ImGui::Text("%s", effect->Name.c_str());
        }

        ImGui::End();
    }
}

static void OnTick(float deltaTime) 
{
    if (!Enabled) 
    {
        return;
    }

    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (pawn && controller && pawn->Health > 0)
    {
        TimeSeconds += deltaTime;

        if (TimeSeconds >= MaxTime)
        {
            TimeSeconds = 0.0f; 
            
            // Temp to only allow one effect to be active 
            if (!ActiveEffects.empty())
            {
                ActiveEffects.clear();
            }

            auto effect = Effects()[rand() % (int)Effects().size()];
            effect->OnStart();

            ActiveEffects.push_back(effect);
        }

        for (auto effect : ActiveEffects)
        {
            effect->OnTick(deltaTime);
        }
    }
}

bool Chaos::Initialize() 
{
    Menu::AddTab("Chaos", ChaosTab);

    Engine::OnTick(OnTick);
    Engine::OnRenderScene(OnRender);

    return true;
}

std::string Chaos::GetName() 
{ 
    return "Chaos"; 
}