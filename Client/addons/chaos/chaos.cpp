#include "chaos.h"
#include "group.h"

#include <codecvt>
#include "../chaos/effect.h"
#include "../../menu.h"
#include "../../settings.h"
#include "../../util.h"

#pragma warning (disable: 26812)

static bool Enabled = false;
static bool Paused = false;
static bool ChaosActive = false;
static std::string LevelName = "";

std::mt19937 rng;
static int Seed = 0;
static bool RandomizeNewSeed = true;

static float DurationTime[static_cast<int>(EDuration::COUNT)] = { 5.0f, 15.0f, 45.0f, 90.0f };
static const char* DurationTimeStrings[] = { "Brief", "Short", "Normal", "Long" };

static float TimeUntilNewEffect = 20.0f;
static float TimeShownInHistory = 40.0f;

static bool DisplayEffectWindow = true;
static bool DisplayTimerWindow = true;
static float TimerInSeconds = 0.0f;
static float TimerHeight = 18.0f;
static ImVec4 TimerBackgroundColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 TimerColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);

struct ActiveEffectInfo {
    float TimeRemaining = 20.0f;
    float HistoryDuration = 40.0f;
    bool ShutdownCorrectly = false;
    Effect* Effect = nullptr;
};

static std::vector<ActiveEffectInfo> ActiveEffects;
static std::vector<Effect*> EnabledEffects;

static void SetNewSeed()
{
    if (RandomizeNewSeed)
    {
        std::random_device rd;
        std::mt19937 rngEngine(rd());
        std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);

        Seed = dist(rngEngine);
        rng.seed(Seed);

        Settings::SetSetting({ "Chaos", "Seed" }, Seed);
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
        if (!active.Effect->Shutdown())
        {
            printf("Chaos: \"%s\" didn't shutdown correctly!\n", active.Effect->Name.c_str());
        }
    }

    ActiveEffects.clear();
    TimerInSeconds = 0.0f;
    ChaosActive = false;

    rng.seed(Seed);
    for (auto effect : Effects())
    {
        effect->SetSeed(Seed);
    }
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
        ImGui::BeginDisabled();
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

    /* TODO: Make UI for group settings
    ImGui::Text("Group Settings");
    {
        //ImGui::HelpMarker("Select a group to toggle the enabled state. You can also enable or disable all effects");
        ImGui::Dummy(ImVec2(0.0f, 2.5f));
        static int selectedGroup = 0;

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

        ImGui::Dummy(ImVec2(0.0f, 2.5f));
        if (ImGui::TreeNode("Toggle Group"))
        {
            for (auto effect : Effects())
            {
                ImGui::Selectable()
                if (effect->GetGroup() & 1 << selectedGroup)
                {
                    effect->Enabled = !effect->Enabled;
                    Settings::SetSetting({ "Chaos", "Effect", effect->Name, "Enabled" }, effect->Enabled);
                }
            }

            ImGui::TreePop();
        }

        ImGui::Separator(2.5f);
    }
    */

    ImGui::Text("Effect Settings");
    {
        ImGui::HelpMarker(
            "Change the duration type and toggle the enabled state. The text below is what group it's in. "
            "You can change these settings while chaos is activated. However, it won't change the setting of active effects"
        );
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

                const auto it = std::find(EnabledEffects.begin(), EnabledEffects.end(), effect);
                if (it != EnabledEffects.end())
                {
                    EnabledEffects.erase(it);
                }
                else
                {
                    EnabledEffects.push_back(effect);
                }
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

    if (!Enabled)
    {
        ImGui::EndDisabled();
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

    if (DisplayTimerWindow)
    {
        const auto window = ImGui::BeginRawScene("##Chaos-TimeCountdownRender");
        const auto& io = ImGui::GetIO();

        window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x, TimerHeight), ImColor(TimerBackgroundColor));
        window->DrawList->AddRectFilled(ImVec2(), ImVec2(io.DisplaySize.x * TimerInSeconds / TimeUntilNewEffect, TimerHeight), ImColor(TimerColor));

        ImGui::EndRawScene();
    }

    if (DisplayEffectWindow)
    {
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350, 165), ImGuiCond_FirstUseEver);
        ImGui::BeginWindow("Effects##Chaos-EffectsUI");

        for (auto it = ActiveEffects.rbegin(); it != ActiveEffects.rend(); it++)
        {
            const auto& active = *it;

            if (active.Effect->Done || active.TimeRemaining <= 0.0f)
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.75f), active.Effect->DisplayName.c_str());
                continue;
            }

            // Don't show the countdown on this effect
            if (active.Effect->GetClass() == "GoToMainMenu")
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
}

static void OnTick(float deltaTime) 
{
    if (!Enabled || !ChaosActive || Paused || LevelName == Map_MainMenu)
    {
        return;
    }

    const auto pawn = Engine::GetPlayerPawn();

    if (!pawn)
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
        std::vector<std::string> activeEffectClasses;
        for (const auto& activeEffect : ActiveEffects)
        {
            activeEffectClasses.push_back(activeEffect.Effect->GetClass());
        }

        std::vector<Effect*> effectPool;
        for (const auto& enabledEffect : EnabledEffects)
        {
            bool addToPool = true;

            for (size_t i = 0; i < activeEffectClasses.size(); i++)
            {
                if (enabledEffect->GetClass() == activeEffectClasses[i])
                {
                    addToPool = false;
                    break;
                }
            }

            if (addToPool)
            {
                effectPool.push_back(enabledEffect);
            }
        }

        std::shuffle(effectPool.begin(), effectPool.end(), rng);

        for (auto effect : effectPool)
        {
            if (!effect->CanActivate())
            {
                printf("Chaos: \"%s\" can't be activated\n", effect->Name.c_str());
                continue;
            }

            ActiveEffectInfo newActiveEffectInfo;
            newActiveEffectInfo.TimeRemaining = DurationTime[(int)effect->DurationType];
            newActiveEffectInfo.HistoryDuration = TimeShownInHistory;
            newActiveEffectInfo.Effect = effect;

            effect->DurationTime = newActiveEffectInfo.TimeRemaining;
            effect->Initialize();

            ActiveEffects.push_back(newActiveEffectInfo);
            break;
        }

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
                printf("Chaos: \"%s\" didn't shutdown correctly!\n", active.Effect->Name.c_str());
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
    Seed = Settings::GetSetting({ "Chaos", "Seed" }, 0);
    RandomizeNewSeed = Settings::GetSetting({ "Chaos", "RandomizeNewSeed" }, true);

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

        if (effect->Enabled)
        {
            EnabledEffects.push_back(effect);
        }
    }

    Menu::AddTab("Chaos", ChaosTab);

    Engine::OnTick(OnTick);
    Engine::OnRenderScene(OnRender);

    Engine::OnPostLevelLoad([](const wchar_t* newLevelName)
    {
        LevelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(newLevelName);
        std::transform(LevelName.begin(), LevelName.end(), LevelName.begin(), [](char c)
        {
            return tolower(c);
        });
    });

    return true;
}

std::string Chaos::GetName() 
{ 
    return "Chaos"; 
}