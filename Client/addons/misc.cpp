#include <codecvt>

#include "../engine.h"
#include "../hook.h"
#include "../imgui/imgui.h"
#include "../menu.h"
#include "../pattern.h"
#include "../settings.h"
#include "../util.h"
#include "misc.h"

static bool enabled = false;
static std::string levelName;

static bool noConsequtiveWallrunsLimit = false;
static bool autorollEnabled = false;
static bool noHealthRegenerationEnabled = false;
static bool permanentReactionTimeEnabled = false;
static bool permanentGameSpeedEnabled = false;
static float permanentGameSpeed = 0.25f;
static bool noWallrunChallenge = false;
static bool noWallclimbChallenge = false;

static bool ohkoEnabled = false;
static EOhko ohkoType = EOhko::Normal;
static int ohkoHealth = 100;

static void MiscTab() { 
    if (ImGui::Checkbox("Enabled", &enabled)) {
        Settings::SetSetting("misc", "enabled", enabled);
    }

    if (!enabled) {
        return;
    }

    auto pawn = Engine::GetPlayerPawn();
    if (!pawn) {
        return;
    }

    auto controller = Engine::GetPlayerController();
    if (!controller) {
        return;
    }

    ImGui::SeperatorWithPadding(2.5f);

    // Auto Roll
    if (ImGui::Checkbox("Auto Roll", &autorollEnabled)) {
        Settings::SetSetting("misc", "autorollEnabled", autorollEnabled);

        if (!autorollEnabled) {
            pawn->RollTriggerTime = 0.0f;
        }
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        if (!ohkoEnabled && !noHealthRegenerationEnabled) {
            ImGui::SetTooltip("Remember the auto roll bug? If enabled, it will auto roll for you");
        } else {
            ImGui::SetTooltip("What's the point of ohko or no health regeneration if auto rolling is on?");
        }
    }

    // Consequtive Wallruns
    if (ImGui::Checkbox("No Consequtive Wallruns Limit", &noConsequtiveWallrunsLimit)) {
        Settings::SetSetting("misc", "consequtiveWallrunsLimitRemoved", noConsequtiveWallrunsLimit);
    }

    // No Wallrun Challenge
    if (ImGui::Checkbox("No Wallrun Challenge", &noWallrunChallenge)) {
        Settings::SetSetting("misc", "noWallrunChallenge", noWallrunChallenge);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        ImGui::SetTooltip("Wallrun and you start from new game");
    }

    // No Wallclimb Challenge
    if (ImGui::Checkbox("No Wallclimb Challenge", &noWallclimbChallenge)) {
        Settings::SetSetting("misc", "noWallclimbChallenge", noWallclimbChallenge);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        ImGui::SetTooltip("Wallclimb and you start from new game");
    }

    // No Health Regeneration
    if (!ohkoEnabled) {
        if (ImGui::Checkbox("No Health Regeneration", &noHealthRegenerationEnabled)) {
            Settings::SetSetting("misc", "noHealthRegenerationEnabled", noHealthRegenerationEnabled);
            
            ohkoHealth = 100;
            pawn->Health = pawn->MaxHealth;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
            ImGui::SetTooltip("Health regeneration is turned off. If you have 100 health and take 15 damage, you'll have 85 health for the rest of the game.\nIf you reach 0 health, it will start a new game for you");
        }
    }

    // Permanent Reaction Time
    if (ImGui::Checkbox("Permanent Reaction Time", &permanentReactionTimeEnabled)) {
        Settings::SetSetting("misc", "permanentReactionTimeEnabled", permanentReactionTimeEnabled);
        Settings::SetSetting("misc", "permanentGameSpeedEnabled", permanentGameSpeedEnabled = false);

        pawn->WorldInfo->TimeDilation = 1.0f;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        ImGui::SetTooltip("Once reactiontime is activated, it will be on forever until faith die, level changed, or deactivated.\nCan't be used in timetrials");
    }

    // Permanent Game Speed
    if (ImGui::Checkbox("Permanent Game Speed", &permanentGameSpeedEnabled)) {
        Settings::SetSetting("misc", "permanentGameSpeedEnabled", permanentGameSpeedEnabled);
        Settings::SetSetting("misc", "permanentReactionTimeEnabled", permanentReactionTimeEnabled = false);

        if (!permanentGameSpeedEnabled) {
            pawn->WorldInfo->TimeDilation = 1.0f; 
        }
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
        ImGui::SetTooltip("Is always active, it will be forever on until deactivated. Can be used in timetrials");
    }

    if (permanentGameSpeedEnabled) {
        if (ImGui::InputFloat("Game Speed", &permanentGameSpeed, 0.1f, 0.5f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
            Settings::SetSetting("misc", "permanentGameSpeed", permanentGameSpeed = max(0.1f, permanentGameSpeed));
        }
    }

    // One Hit Knock Down
    if (!noHealthRegenerationEnabled) 
    {
        if (ImGui::Checkbox("Ohko", &ohkoEnabled)) {
            Settings::SetSetting("misc", "ohkoEnabled", ohkoEnabled);

            ohkoHealth = 100;
            pawn->Health = pawn->MaxHealth;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
            ImGui::SetTooltip("One Hit Knock Out");
        }

        // Ohko Type Settings
        if (ohkoEnabled) {
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Text("Ohko Type");

            if (ImGui::RadioButton("Normal", ohkoType == EOhko::Normal)) {
                Settings::SetSetting("misc", "ohkoType", ohkoType = EOhko::Normal);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
                ImGui::SetTooltip("If you take any damage at all, you'll respawn at the last checkpoint");
            }

            if (ImGui::RadioButton("Extreme", ohkoType == EOhko::Extreme)) {
                Settings::SetSetting("misc", "ohkoType", ohkoType = EOhko::Extreme);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
                ImGui::SetTooltip("If you take any damage at all, it will instantly start a new game");
            }
        }
    }
}

static void OnRender(IDirect3DDevice9 *) {
    if (!enabled) {
        return;
    }
}

static void StartNewGame() {
    auto const gameInfo = static_cast<Classes::ATdGameInfo *>(Engine::GetWorld()->Game);
    gameInfo->TdGameData->StartNewGameWithTutorial(true);

    ohkoHealth = 100;
}

static void OnTick(float deltaTime) {
    if (!enabled) {
        return;
    }

    auto world = Engine::GetWorld();
    if (!world) {
        return;
    }

    if (!permanentReactionTimeEnabled && permanentGameSpeedEnabled) {
        world->TimeDilation = permanentGameSpeed;
    }

    auto pawn = Engine::GetPlayerPawn();
    if (!pawn) {
        return;
    }

    auto controller = Engine::GetPlayerController();
    if (!controller) {
        return;
    }

    if (!permanentReactionTimeEnabled && permanentGameSpeedEnabled) {
        controller->ReactionTimeEnergy = 0.0f;
    }

    if (permanentReactionTimeEnabled && !permanentGameSpeedEnabled) {
        if (controller->bReactionTime && controller->ReactionTimeEnergy <= 50.f) {
            controller->ReactionTimeEnergy = 50.f;
        }
    }

    if (levelName.empty()) {
        levelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(
            pawn->WorldInfo->GetMapName(false).c_str());

        std::transform(levelName.begin(), levelName.end(),
                       levelName.begin(), [](char c) { return tolower(c); });
    }

    if (ohkoEnabled && levelName != Map_MainMenu &&
        (pawn->EnterFallingHeight != -1e30f && pawn->Health != pawn->MaxHealth)) {
        if (ohkoType == EOhko::Normal) {
            pawn->RegenerateDelay = 5;
            pawn->RegenerateHealthPerSecond = 25;

            if (pawn->Health != pawn->MaxHealth) {
                pawn->Suicide();
            }
        } else if (ohkoType == EOhko::Extreme) {
            if (pawn->Health != pawn->MaxHealth) {
                StartNewGame();
            }
        }
    }

    if (noHealthRegenerationEnabled && levelName != Map_MainMenu &&
        (pawn->EnterFallingHeight != -1e30f && pawn->Health != pawn->MaxHealth)) {

        pawn->RegenerateDelay = 0;
        pawn->RegenerateHealthPerSecond = 0;

        if (ohkoHealth != pawn->Health) {
            if (pawn->Health == pawn->MaxHealth) {
                pawn->Health = ohkoHealth;
            } else if (pawn->Health <= 0) {
                StartNewGame();
            } else {
                if (ohkoHealth > pawn->Health) {
                    ohkoHealth = pawn->Health;
                }

                if (ohkoHealth <= 0) {
                    StartNewGame();
                }
            }
        }
    }

    if (noConsequtiveWallrunsLimit) {
        if (pawn->MovementState == Classes::EMovement::MOVE_WallRunningLeft ||
            pawn->MovementState == Classes::EMovement::MOVE_WallRunningRight) {

            const auto wallrun = static_cast<Classes::UTdMove_WallRun *>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_WallRunningRight)]);

            if (wallrun) {
                if (wallrun->ConsequtiveWallruns > 0) {
                    wallrun->ConsequtiveWallruns = 0;
                }
            }
        }
    }

    if (autorollEnabled) {
        pawn->RollTriggerTime = 1e30f;
    }

    if (noWallrunChallenge) {
        if (pawn->MovementState == Classes::EMovement::MOVE_WallRunningLeft ||
            pawn->MovementState == Classes::EMovement::MOVE_WallRunningRight) {
            StartNewGame();
        }
    }

    if (noWallclimbChallenge) {
        if (pawn->MovementState == Classes::EMovement::MOVE_WallClimbing) {
            StartNewGame();
        }
    }
}

bool Misc::Initialize() {
    enabled = Settings::GetSetting("misc", "enabled", false);

    noConsequtiveWallrunsLimit = Settings::GetSetting("misc", "consequtiveWallrunsLimitRemoved", false);
    autorollEnabled = Settings::GetSetting("misc", "autorollEnabled", false);
    permanentReactionTimeEnabled = Settings::GetSetting("misc", "permanentReactionTimeEnabled", false);
    permanentGameSpeedEnabled = Settings::GetSetting("misc", "permanentGameSpeedEnabled", false);
    permanentGameSpeed = Settings::GetSetting("misc", "permanentGameSpeed", 0.25f);
    noWallrunChallenge = Settings::GetSetting("misc", "noWallrunChallenge", false);
    noWallclimbChallenge = Settings::GetSetting("misc", "noWallclimbChallenge", false);

    ohkoEnabled = Settings::GetSetting("misc", "ohkoEnabled", false);
    ohkoType = Settings::GetSetting("misc", "ohkoType", EOhko::Normal).get<EOhko>();

    Menu::AddTab("Misc", MiscTab);
    Engine::OnTick(OnTick);
    Engine::OnRenderScene(OnRender);

    Engine::OnPostLevelLoad([](const wchar_t *newLevelName) {
        levelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(newLevelName);
        std::transform(levelName.begin(), levelName.end(), levelName.begin(), [](char c) { return tolower(c); });
    });

    return true;
}

std::string Misc::GetName() { return "Misc"; }