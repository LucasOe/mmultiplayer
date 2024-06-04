#include "chaos.h"

#include "../chaos/effect.h"
#include "../../menu.h"
#include "../../settings.h"
#include "../../util.h"

static bool IsEnabled = false;
static bool IsPaused = false;

static bool EffectRandomizerHasBeenSeeded = false;
static int Seed = 0;
std::mt19937 rng;

static float DurationTime[3] = { 15.0f, 45.0f, 90.0f };
static float TimerInSeconds = 0.0f;
static float TimeUntilNewRandomEffect = 20.0f;

static float TimerHeight = 18.0f;
static float TimerOffsetTop = 0.0f;
static ImVec4 BackgroundColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 TimerColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);

struct ActiveEffectInfo {
    float TimeRemaining;
    bool ShutdownSuccessfully;
    Effect* Effect;
};

std::vector<ActiveEffectInfo> ActiveEffects;

static void Restart()
{
    for (auto active : ActiveEffects)
    {
        active.Effect->Shutdown();
    }

    ActiveEffects.clear();
    ActiveEffects.shrink_to_fit();

    TimerInSeconds = 0.0f;
    IsPaused = true;
    EffectRandomizerHasBeenSeeded = false;
}

static void ChaosTab()
{
    if (ImGui::Checkbox("Enabled##Chaos-Enabled", &IsEnabled))
    {
        Restart();
    }

    if (!IsEnabled)
    {
        return;
    }

    ImGui::Separator(2.5f);
    char buffer[0xFF];
    sprintf_s(buffer, sizeof(buffer), "%s##Chaos-ResumeOrPauseButton", IsPaused ? "Resume" : "Pause");

    if (ImGui::Button(buffer))
    {
        IsPaused = !IsPaused;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Restart##Chaos-RestartButton"))
    {
        Restart();
    }

    ImGui::SameLine();
    if (ImGui::Button("Randomize Seed"))
    {
        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
        Seed = dist(rng);

        EffectRandomizerHasBeenSeeded = false;

        for (auto effect : Effects())
        {
            effect->SetSeed(Seed);
        }
    }

    if (ImGui::InputInt("Seed##Chaos-Seed", &Seed, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        EffectRandomizerHasBeenSeeded = false;

        for (auto effect : Effects())
        {
            effect->SetSeed(Seed);
        }
    }

    ImGui::Separator(2.5f);

    if (ImGui::SliderFloat("Time Until New Effect##Chaos-MaxTime", &TimeUntilNewRandomEffect, 5.0f, 60.0f, "%.0f sec"))
    {
        TimerInSeconds = 0.0f;
    }

    const auto& io = ImGui::GetIO();

    ImGui::SliderFloat("Timer Height##Chaos-Timer-Height", &TimerHeight, 1.0f, 30.0f, "%.1f");
    ImGui::ColorEdit4("Background Color##Chaos-Timer-BackgroundColor", &BackgroundColor.x);
    ImGui::ColorEdit4("Timer Color##Chaos-Timer-Color", &TimerColor.x);
    ImGui::Separator(2.5f);

    ImGui::SliderFloat("Short Duration Time##Chaos-ShortDurationTime", &DurationTime[0], 5.0f, 120.0f, "%.0f sec");
    ImGui::SliderFloat("Normal Duration Time##Chaos-NormalDurationTime", &DurationTime[1], 5.0f, 120.0f, "%.0f sec");
    ImGui::SliderFloat("Long Duration Time##Chaos-LongDurationTime", &DurationTime[2], 5.0f, 120.0f, "%.0f sec");
    ImGui::Separator(2.5f);

    // TODO: Show all effects and choose which to enable or disable
    // There should be a button to disable all of the same type of effect if there are multiple ones
    for (auto effect : Effects())
    {
        ImGui::Checkbox(effect->Name.c_str(), &effect->IsEnabled);
    }
}

static void OnRender(IDirect3DDevice9* device) 
{
    if (!IsEnabled)
    {
        return;
    }

    for (auto active : ActiveEffects)
    {
        if (active.TimeRemaining >= 0.0f)
        {
            active.Effect->Render(device);
        }
    }

    const auto window = ImGui::BeginRawScene("##Chaos-Timer-Render");
    const auto& io = ImGui::GetIO();

    window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x, TimerHeight), ImColor(BackgroundColor));
    window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x * TimerInSeconds / TimeUntilNewRandomEffect, TimerHeight), ImColor(TimerColor));

    ImGui::EndRawScene();

    // Temp until proper UI
    ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(285, 165), ImGuiCond_FirstUseEver);
    ImGui::Begin("Active Effects");

    for (auto it = ActiveEffects.rbegin(); it != ActiveEffects.rend(); ++it)
    {
        const auto& active = *it;

        if (active.Effect->IsDone)
        {
            ImGui::Text("%s", active.Effect->DisplayName.c_str());
        }
        else
        {
            ImGui::Text("%s (%.1f)", active.Effect->DisplayName.c_str(), active.TimeRemaining);
        }
    }

    ImGui::End();
}

static void AddRandomEffect()
{
    if (!EffectRandomizerHasBeenSeeded)
    {
        rng.seed(Seed); 
        EffectRandomizerHasBeenSeeded = true;
    }

    std::uniform_int_distribution<int> dist(0, Effects().size() - 1);

    for (int attempt = 0; attempt < 100; attempt++)
    {
        auto effect = Effects()[dist(rng)];

        if (!effect->IsEnabled)
        {
            continue;
        }

        bool isDuplicate = false;
        for (const auto& active : ActiveEffects)
        {
            if (effect->GetType() == active.Effect->GetType())
            {
                isDuplicate = true;
                break;
            }
        }

        if (isDuplicate)
        {
            continue;
        }

        ActiveEffectInfo newActiveEffectInfo;
        newActiveEffectInfo.TimeRemaining = DurationTime[(int)effect->DurationType];
        newActiveEffectInfo.ShutdownSuccessfully = false;
        newActiveEffectInfo.Effect = effect;

        effect->DurationTimeAllocated = newActiveEffectInfo.TimeRemaining;
        effect->Start();

        ActiveEffects.push_back(newActiveEffectInfo);
        break;
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

    TimerInSeconds += deltaTime;

    if (TimerInSeconds >= TimeUntilNewRandomEffect)
    {
        AddRandomEffect();
        TimerInSeconds = 0.0f;
    }

    for (auto it = ActiveEffects.rbegin(); it != ActiveEffects.rend(); ++it)
    {
        auto& active = *it;
        active.TimeRemaining -= deltaTime;

        if (active.TimeRemaining >= 0.0f)
        {
            active.Effect->Tick(deltaTime);
            continue;
        }

        if (!active.ShutdownSuccessfully)
        {
            if (!active.Effect->Shutdown())
            {
                printf("Chaos: The effect \"%s\" didn't shut down correctly and might persist\n", active.Effect->Name.c_str());
            }
            else
            {
                active.ShutdownSuccessfully = true;
                ActiveEffects.erase((it + 1).base());
            }
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