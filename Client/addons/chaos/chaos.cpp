#include "chaos.h"
#include "group.h"

#include "../chaos/effect.h"
#include "../../menu.h"
#include "../../settings.h"
#include "../../util.h"

#pragma warning (disable: 26812)

static bool Enabled = false;
static bool Paused = false;
static bool ChaosActive = false;

std::mt19937 rng;
static int Seed = 0;
static bool RandomizeNewSeed = true;

static float DurationTime[static_cast<int>(EDuration::COUNT)] = { 5.0f, 15.0f, 45.0f, 90.0f };
static const char* DurationTimeStrings[] = { "Brief", "Short", "Normal", "Long" };

static float TimeUntilNewEffect = 20.0f;
static float TimeShownInHistory = 40.0f;
static int MaxAttemptsForNewEffect = 100;

static float TimerInSeconds = 0.0f;
static float TimerHeight = 18.0f;
static ImVec4 TimerBackgroundColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 TimerColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);

struct ActiveEffectInfo {
    float TimeRemaining;
    float HistoryDuration;
    bool ShutdownCorrectly;
    Effect* Effect;
};

static std::vector<ActiveEffectInfo> ActiveEffects;

static void SetNewSeed()
{
    if (RandomizeNewSeed)
    {
        std::random_device rd;
        std::mt19937 rngEngine(rd());
        std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);

        Seed = dist(rngEngine);
        rng.seed(Seed);

        Settings::SetSetting({ "Chaos", "Settings", "Seed" }, Seed);
    }

    for (auto effect : Effects())
    {
        effect->SetSeed(Seed);
    }
}

static void Restart()
{
    for (auto active : ActiveEffects)
    {
        active.Effect->Shutdown();
    }

    ActiveEffects.clear();

    TimerInSeconds = 0.0f;
    ChaosActive = false;
}

static std::string GetGroupNames(EGroup_ group)
{
    std::vector<std::string> names;

    if (group == EGroup_None)
    {
        return GroupNames[0];
    }

    for (int i = 0; i < IM_ARRAYSIZE(GroupNames); i++)
    {
        if (group & 1 << i)
        {
            names.push_back(GroupNames[i]);
        }
    }

    std::string result;
    for (size_t i = 0; i < names.size(); i++)
    {
        result += names[i];
        if (i < names.size() - 1)
        {
            result += ", ";
        }
    }

    return result;
}

static void ChaosTab()
{
    if (ImGui::Checkbox("Enabled##Chaos-EnabledCheckbox", &Enabled))
    {
        Settings::SetSetting({ "Chaos", "Enabled" }, Enabled);

        if (!Enabled)
        {
            Restart();
        }
    }

    if (!Enabled)
    {
        return;
    }

    if (ImGui::Checkbox("Randomize Seed##Chaos-RandomizeNewSeed", &RandomizeNewSeed))
    {
        Settings::SetSetting({ "Chaos", "RandomizeNewSeed" }, RandomizeNewSeed);
    }

    ImGui::Separator(2.5f);

    if (ChaosActive)
    {
        if (ImGui::Button(Paused ? "Resume##Chaos-ResumeButton" : "Pause##Chaos-PauseButton"))
        {
            Paused = !Paused;
        }

        ImGui::SameLine();
        if (ImGui::Button("Restart##Chaos-RestartButton"))
        {
            Restart();
        }
    }
    else
    {
        if (ImGui::Button("Start##Chaos-StartButton"))
        {
            SetNewSeed();
            Restart();

            Paused = false;
            ChaosActive = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Start New Game##Chaos-StartNewGameButton"))
        {
            SetNewSeed();
        
            const auto world = Engine::GetWorld();

            if (world)
            {
                const auto gameInfo = static_cast<Classes::ATdGameInfo*>(world->Game);
                gameInfo->TdGameData->StartNewGameWithTutorial(true);
            
                Restart();
                ChaosActive = true;
            }
        }
    }

    if (RandomizeNewSeed)
    {
        ImGui::BeginDisabled();
        ImGui::InputInt("Seed##Chaos-Seed", &Seed, 0, 0);
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::InputInt("Seed##Chaos-Seed", &Seed, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            SetNewSeed();
        }
    }

    ImGui::Separator(2.5f);

    ImGui::Text("Timer");
    {
        //ImGui::HelpMarker("");
        ImGui::Dummy(ImVec2(0.0f, 2.5f));

        if (ImGui::SliderFloat("Time Until New Effect##Chaos-TimeUntilNewEffect", &TimeUntilNewEffect, 5.0f, 60.0f, "%.0f sec", ImGuiSliderFlags_AlwaysClamp))
        {
            Settings::SetSetting({ "Chaos", "Timer", "TimeUntilNewEffect" }, TimeUntilNewEffect);
        }

        if (ImGui::SliderFloat("Time Shown In History##Chaos-TimeShownInHistory", &TimeShownInHistory, 5.0f, 60.0f, "%.0f sec", ImGuiSliderFlags_AlwaysClamp))
        {
            Settings::SetSetting({ "Chaos", "Timer", "TimeShownInHistory" }, TimeShownInHistory);
        }

        if (ImGui::SliderFloat("Timer Height##Chaos-TimerHeight", &TimerHeight, 0.0f, 30.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
        {
            Settings::SetSetting({ "Chaos", "Timer", "TimerHeight" }, TimerHeight);
        }

        ImGui::ColorEdit4("Timer Color##Chaos-TimerColor", &TimerColor.x);
        ImGui::ColorEdit4("Background Color##Chaos-TimerBackgroundColor", &TimerBackgroundColor.x);
        ImGui::Separator(2.5f);
    }

    ImGui::Text("Duration Time");
    {
        //ImGui::HelpMarker("");
        ImGui::Dummy(ImVec2(0.0f, 2.5f));

        for (int i = 0; i < static_cast<int>(EDuration::COUNT); i++)
        {
            const auto label = std::string(DurationTimeStrings[i]) + "##Chaos-" + DurationTimeStrings[i] + "-DurationTime";
            if (ImGui::SliderFloat(label.c_str(), &DurationTime[i], 5.0f, 120.0f, "%.0f sec"))
            {
                DurationTime[i] = ImClamp(DurationTime[i], 5.0f, 3600.0f);
                Settings::SetSetting({ "Chaos", "Duration", DurationTimeStrings[i] }, TimeShownInHistory);
            }
        }

        ImGui::Separator(2.5f); 
    }

    ImGui::Text("Group Settings");
    {
        //ImGui::HelpMarker("Select a group to toggle the enabled state. You can also enable or disable all effects");
        ImGui::Dummy(ImVec2(0.0f, 2.5f));
        static int selectedGroup = 0;

        ImGui::SetNextItemWidth(128.0f);
        if (ImGui::Combo("##Chaos-ToggleGroup", &selectedGroup, GroupNames, IM_ARRAYSIZE(GroupNames)))
        {
            for (auto effect : Effects())
            {
                if (effect->GetGroup() & 1 << selectedGroup)
                {
                    effect->Enabled = !effect->Enabled;
                    Settings::SetSetting({ "Chaos", "Effect", effect->Name, "Enabled" }, effect->Enabled);
                }
            }
        }

        bool clicked = false;
        bool newState = false;
        if (ImGui::Button("Enable All Effects##Chaos-EnableAllEffects"))
        {
            clicked = true;
            newState = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Disable All Effects##Chaos-DisableAllEffects"))
        {
            clicked = true;
            newState = false;
        }
    
        if (clicked)
        {
            for (auto effect : Effects())
            {
                effect->Enabled = newState;
                Settings::SetSetting({ "Chaos", "Effect", effect->Name, "Enabled" }, effect->Enabled);
            }
        }

        ImGui::Separator(2.5f);
    }

    ImGui::Text("Effect Settings");
    {
        //ImGui::HelpMarker("Change the duration type and toggle the enabled state. The text below is what group it's in.");
        ImGui::Dummy(ImVec2(0.0f, 2.5f));

        for (auto effect : Effects())
        {
            if (!effect->Enabled)
            {
                ImGui::BeginDisabled();
            }

            int durationType = (int)effect->DurationType;
            ImGui::SetNextItemWidth(128.0f);
            if (ImGui::Combo(("##Chaos-DurationType-" + effect->Name).c_str(), &durationType, DurationTimeStrings, IM_ARRAYSIZE(DurationTimeStrings)))
            {
                effect->DurationType = static_cast<EDuration>(durationType);
                effect->DurationTime = DurationTime[(int)effect->DurationType];

                Settings::SetSetting({ "Chaos", "Effect", effect->Name, "DurationType" }, durationType);
            }

            if (!effect->Enabled)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (ImGui::Checkbox(effect->Name.c_str(), &effect->Enabled))
            {
                Settings::SetSetting({ "Chaos", "Effect", effect->Name, "Enabled" }, effect->Enabled);
            }

            if (!effect->Enabled)
            {
                ImGui::BeginDisabled();
                ImGui::Text(GetGroupNames(static_cast<EGroup_>(effect->GetGroup())).c_str());
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::Text(GetGroupNames(static_cast<EGroup_>(effect->GetGroup())).c_str());
            }

            ImGui::Dummy(ImVec2(0.0f, 2.5f));
        }
    }
}

static void OnRender(IDirect3DDevice9* device) 
{
    if (!Enabled)
    {
        return;
    }

    for (auto active : ActiveEffects)
    {
        if (active.TimeRemaining >= 0.0f && !active.Effect->Done)
        {
            active.Effect->Render(device);
        }
    }

    const auto window = ImGui::BeginRawScene("##Chaos-TimeCountdownRender");
    const auto& io = ImGui::GetIO();

    window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x, TimerHeight), ImColor(TimerBackgroundColor));
    window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x * TimerInSeconds / TimeUntilNewEffect, TimerHeight), ImColor(TimerColor));

    ImGui::EndRawScene();

    // Temp until proper UI
    ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 165), ImGuiCond_FirstUseEver);
    ImGui::BeginWindow("Effects##Chaos-EffectsTempUI");

    for (auto it = ActiveEffects.rbegin(); it != ActiveEffects.rend(); it++)
    {
        const auto& active = *it;

        if (active.Effect->Done || active.TimeRemaining <= 0.0f)
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.75f), active.Effect->DisplayName.c_str());
            continue;
        }

        if (active.Effect->GetType() == "GoToMainMenu")
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
    std::uniform_int_distribution<int> dist(0, Effects().size() - 1);

    for (int i = 0; i < MaxAttemptsForNewEffect; i++)
    {
        auto effect = Effects()[dist(rng)];

        if (!effect->Enabled)
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
        newActiveEffectInfo.HistoryDuration = TimeShownInHistory;
        newActiveEffectInfo.Effect = effect;

        effect->DurationTime = newActiveEffectInfo.TimeRemaining;
        effect->Start();

        ActiveEffects.push_back(newActiveEffectInfo);
        break;
    }
}

static void OnTick(float deltaTime) 
{
    if (!Enabled || !ChaosActive || Paused)
    {
        return;
    }

    const auto pawn = Engine::GetPlayerPawn();
    const auto controller = Engine::GetPlayerController();

    if (!pawn || !controller)
    {
        return;
    }

    const float timeSinceSpawned = pawn->WorldInfo->TimeSeconds - pawn->SpawnTime;
    const float timeSinceCreated = pawn->WorldInfo->TimeSeconds - pawn->CreationTime;
    
    const auto engine = Engine::GetEngine();
    const float maxDeltaTime = 1.0f / (engine ? engine->MinSmoothedFrameRate : 22.0f);

    if (timeSinceSpawned == 0.0f || timeSinceCreated == 0.0f || deltaTime >= maxDeltaTime)
    {
        return;
    }

    TimerInSeconds += deltaTime;

    if (TimerInSeconds >= TimeUntilNewEffect)
    {
        AddRandomEffect();
        TimerInSeconds = 0.0f;
    }

    for (auto it = ActiveEffects.rbegin(); it != ActiveEffects.rend(); it++)
    {
        auto& active = *it;
        active.TimeRemaining -= deltaTime;

        if (active.TimeRemaining >= 0.0f && !active.Effect->Done)
        {
            active.Effect->Tick(deltaTime);
            continue;
        }

        if (!active.ShutdownCorrectly)
        {
            active.ShutdownCorrectly = active.Effect->Shutdown();
            if (!active.ShutdownCorrectly)
            {
                printf("The effect \"%s\" didn't shutdown correctly!\n", active.Effect->Name.c_str());
                continue;
            }
        }

        active.HistoryDuration -= deltaTime;
        if (active.HistoryDuration >= 0.0f)
        {
            continue;
        }

        ActiveEffects.erase((it + 1).base());
    }
}

bool Chaos::Initialize() 
{
    Enabled = Settings::GetSetting({ "Chaos", "Enabled" }, false);
    TimeUntilNewEffect = Settings::GetSetting({ "Chaos", "Timer", "TimeUntilNewEffect" }, 20.0f);
    TimeShownInHistory = Settings::GetSetting({ "Chaos", "Timer", "TimeShownInHistory" }, 40.0f);
    TimerHeight = Settings::GetSetting({ "Chaos", "Timer", "TimerHeight" }, 18.0f);

    TimerColor = JsonToImVec4(Settings::GetSetting({ "Chaos", "Timer", "TimerColor" }, ImVec4ToJson(ImVec4(0.0f, 0.5f, 1.0f, 1.0f))));
    TimerBackgroundColor = JsonToImVec4(Settings::GetSetting({ "Chaos", "Timer", "TimerBackgroundColor" }, ImVec4ToJson(ImVec4(0.0f, 0.0f, 0.0f, 1.0f))));

    for (int i = 0; i < static_cast<int>(EDuration::COUNT); i++)
    {
        DurationTime[i] = Settings::GetSetting({ "Chaos", "Duration", DurationTimeStrings[i] }, DurationTime[i]);
    }

    for (auto effect : Effects())
    {
        effect->Enabled = Settings::GetSetting({ "Chaos", "Effect", effect->Name, "Enabled" }, true);
        effect->DurationType = Settings::GetSetting({ "Chaos", "Effect", effect->Name, "DurationType" }, effect->DurationType).get<EDuration>();
        
        // Clamp it between 0 and Count - 1. If not clamped, it would not get the durationtime
        effect->DurationType = static_cast<EDuration>(ImClamp(static_cast<int>(effect->DurationType), 0, static_cast<int>(EDuration::COUNT) - 1));
        effect->DurationTime = DurationTime[(int)effect->DurationType];
    }

    Menu::AddTab("Chaos", ChaosTab);

    Engine::OnTick(OnTick);
    Engine::OnRenderScene(OnRender);

    return true;
}

std::string Chaos::GetName() 
{ 
    return "Chaos"; 
}