#include "chaos.h"

#include "../chaos/effect.h"
#include "../../menu.h"
#include "../../settings.h"
#include "../../util.h"

static bool IsEnabled = false;
static bool IsPaused = false;

static float MaxTime = 20.0f;
static float TimerHeight = 18.0f;
static float TimerOffsetTop = 0.0f;
static ImVec4 BackgroundColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 TimerColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);

static float TimeSeconds;
std::vector<Effect*> ActiveEffects;

static void ChaosTab() 
{
    ImGui::Checkbox("Is Enabled##Chaos-Enabled", &IsEnabled);
    ImGui::Checkbox("Is Paused##Chaos-Paused", &IsPaused);

    if (ImGui::SliderFloat("Time##Chaos-MaxTime", &MaxTime, 5.0f, 60.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
    {
        TimeSeconds = 0.0f;
    }

    const auto& io = ImGui::GetIO();

    ImGui::SliderFloat("Timer Height##Chaos-Timer-Height", &TimerHeight, 1.0f, 30.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::ColorEdit4("Background Color##Chaos-Timer-BackgroundColor", &BackgroundColor.x);
    ImGui::ColorEdit4("Timer Color##Chaos-Timer-Color", &TimerColor.x);
    ImGui::Separator(2.5f);

    for (auto effect : Effects())
    {
        ImGui::Text("%s (%s)", effect->Name.c_str(), effect->GetType().c_str());
    }
}

static void OnRender(IDirect3DDevice9*) 
{
    if (!IsEnabled)
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
    if (!IsEnabled || IsPaused)
    {
        return;
    }

    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (!pawn || !controller)
    {
        return;
    }

    TimeSeconds += deltaTime;

    if (TimeSeconds >= MaxTime)
    {
        TimeSeconds = 0.0f;

        for (int attempt = 0; attempt < 100; attempt++)
        {
            auto effect = Effects()[rand() % (int)Effects().size()];

            bool isDuplicate = false;
            for (auto activeEffect : ActiveEffects)
            {
                if (effect->GetType() == activeEffect->GetType())
                {
                    isDuplicate = true;
                    break;
                }
            }

            if (isDuplicate == false)
            {
                effect->TimeLeftInSeconds = MaxTime * 3;
                effect->Start();

                ActiveEffects.push_back(effect);
                break;
            }
        }
    }

    for (auto it = ActiveEffects.rbegin(); it != ActiveEffects.rend(); ++it)
    {
        auto effect = *it;
        effect->TimeLeftInSeconds -= deltaTime;

        if (effect->TimeLeftInSeconds >= 0.0f)
        {
            effect->Tick(deltaTime);
        }
        else
        {
            if (!effect->Shutdown())
            {
                printf("Chaos: The effect \"%s\" didn't shut down correctly and might persist\n", effect->Name.c_str());
            }

            ActiveEffects.erase((it + 1).base());
        }
    }

    ActiveEffects.shrink_to_fit();
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