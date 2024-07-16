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

// Global variable, used for effects to get the level name
std::string LevelName = "";

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

static void ToggleEffects(const bool enable, const int groupIndex)
{
    EGroup_ group;

    if (groupIndex != -1)
    {
        group = static_cast<EGroup_>(1 << groupIndex);
    }

    for (auto effect : Effects())
    {
        if (groupIndex != -1 && (effect->GetGroup() & group) == 0)
        {
            continue;
        }

        const auto it = std::find(EnabledEffects.begin(), EnabledEffects.end(), effect);

        if (enable)
        {
            if (it == EnabledEffects.end())
            {
                EnabledEffects.push_back(effect);
            }
        }
        else
        {
            if (it != EnabledEffects.end())
            {
                EnabledEffects.erase(it);
            }
        }

        effect->Enabled = enable;
        Settings::SetSetting({ "Chaos", "Effect", effect->Name, "Enabled" }, effect->Enabled);
    }
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
        if (ImGui::SliderFloat("Time Until New Effect##Chaos-TimeUntilNewEffect", &TimeUntilNewEffect, 5.0f, 60.0f, "%.0f sec"))
        {
            TimeUntilNewEffect = ImMax(5.0f, TimeUntilNewEffect);
            Settings::SetSetting({ "Chaos", "Timer", "TimeUntilNewEffect" }, TimeUntilNewEffect);
        }

        if (ImGui::SliderFloat("Time Shown In History##Chaos-TimeShownInHistory", &TimeShownInHistory, 5.0f, 60.0f, "%.0f sec"))
        {
            TimeShownInHistory = ImMax(5.0f, TimeShownInHistory);
            Settings::SetSetting({ "Chaos", "Timer", "TimeShownInHistory" }, TimeShownInHistory);
        }

        if (ImGui::SliderFloat("Timer Height##Chaos-TimerHeight", &TimerHeight, 0.0f, 30.0f, "%.0f"))
        {
            TimerHeight = ImMax(0.0f, TimerHeight);
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

    ImGui::Text("Group Settings");
    {
        ImGui::HelpMarker(
            "Select a group to toggle the enabled state. Scroll down to see what each effect has for group flags. "
            "You can also enable or disable all effects. There's no undo for this"
        );

        // ImGui::Text("Effects().size(): %d", Effects().size());
        // ImGui::Text("EnabledEffects.size(): %d", EnabledEffects.size());

        static int groupIndex = 0;
        const char* previewValue = GroupNames[groupIndex];

        ImGui::SetNextItemWidth(128.0f);
        if (ImGui::BeginCombo("Selected Group", previewValue))
        {
            for (int i = 0; i < IM_ARRAYSIZE(GroupNames); i++)
            {
                const bool isSelected = (groupIndex == i);
                if (ImGui::Selectable(GroupNames[i], isSelected))
                {
                    groupIndex = i;
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Dummy(ImVec2(0.0f, 2.5f));

        char buffer[0x80];
        sprintf_s(buffer, sizeof(buffer), "Enable All \"%s\" Effects##Chaos-EnableAllSpecificGroup", GroupNames[groupIndex]);
        if (ImGui::Button(buffer))
        {
            ToggleEffects(true, groupIndex);
        }

        sprintf_s(buffer, sizeof(buffer), "Disable All \"%s\" Effects##Chaos-DisableAllSpecificGroup", GroupNames[groupIndex]);
        if (ImGui::Button(buffer))
        {
            ToggleEffects(false, groupIndex);
        }

        ImGui::Dummy(ImVec2(0.0f, 2.5f));
        if (ImGui::Button("Enable All Effects##Chaos-EnableAllEffects"))
        {
            ToggleEffects(true, -1);
        }

        if (ImGui::Button("Disable All Effects##Chaos-DisableAllEffects"))
        {
            ToggleEffects(false, -1);
        }

        ImGui::Separator(2.5f);
    }

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
                ToggleEffects(&effect->Enabled, -1);
            }

            const auto groupNames = GetGroupNames(static_cast<EGroup_>(effect->GetGroup()));

            if (!effect->Enabled)
            {
                ImGui::BeginDisabled();
                ImGui::Text(groupNames.c_str());
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::Text(groupNames.c_str());
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

            ImGui::Text("%s (%.1f)", active.Effect->DisplayName.c_str(), active.TimeRemaining);
        }

        ImGui::End();
    }
}

static bool IsUISceneOpened(const std::vector<std::string>& names)
{
    const auto viewport = Engine::GetViewportClient();
    if (!viewport || !viewport->UIController)
    {
        return false;
    }

    const auto scene = viewport->UIController->SceneClient;
    if (!scene)
    {
        return false;
    }

    for (size_t i = 0; i < scene->ActiveScenes.Num(); i++)
    {
        const auto currentScene = scene->ActiveScenes[i];
        if (!currentScene)
        {
            continue;
        }

        const auto currentSceneName = currentScene->GetObjectName();

        for (size_t j = 0; j < names.size(); j++)
        {
            if (currentSceneName == names[j])
            {
                return true;
            }
        }
    }

    return false;
}

static void OnTick(float deltaTime) 
{
    if (!Enabled || !ChaosActive || Paused || LevelName == Map_MainMenu)
    {
        return;
    }

    if (IsUISceneOpened({ "TdGameObjectives", "TdSPPause", "TdTimeTrialPause", "SpLevelRacePause" }))
    {
        return;
    }

    const auto controller = Engine::GetPlayerController();
    if (!controller || controller->bCinematicMode || controller->bDisableSkipCutscenes)
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
        if (active.HistoryDuration <= 0.0f)
        {
            ActiveEffects.erase((it + 1).base());
        }
    }

    TimerInSeconds += deltaTime;

    if (TimerInSeconds <= TimeUntilNewEffect)
    {
        return;
    }

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

        ActiveEffectInfo newEffect;
        newEffect.TimeRemaining = DurationTime[(int)effect->DurationType];
        newEffect.HistoryDuration = TimeShownInHistory;
        newEffect.Effect = effect;

        effect->DurationTime = newEffect.TimeRemaining;
        effect->Initialize();

        ActiveEffects.push_back(newEffect);
        break;
    }

    TimerInSeconds = 0.0f;
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