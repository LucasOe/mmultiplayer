
#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#include <codecvt>

#include "../engine.h"
#include "../imgui/imgui.h"
#include "../menu.h"
#include "../settings.h"
#include "../util.h"
#include "misc.h"

#include "../string_utils.h"

static bool Enabled = false;
static std::string LevelName;

static bool ConsequtiveWallrunsLimitRemoved = false;
static bool AutoLockDisabled = false;
static bool AutoRollEnabled = false;
static bool PermanentReactionTimeEnabled = false;
static bool PermanentGameSpeedEnabled = false;
static float PermanentGameSpeed = 0.25f;
static bool NoWallrunChallenge = false;
static bool NoWallclimbChallenge = false;
static bool NoHealthRegenerationEnabled = false;

static bool TunnelVisionEnabled = false;
static bool TunnelVisionInverted = false;
static ImVec2 TunnelVisionSize = ImVec2(128.0f, 128.0f);

static bool CustomColorScaleEnabled = false;
static Classes::FVector CustomColorScaleValues = Classes::FVector{1.0f, 1.0f, 1.0f};

enum class EOhko : uint8_t 
{
    Normal,
    Extreme,
};

static struct
{
    bool Enabled = false;
    EOhko Type = EOhko::Normal;
    int Health = 100;
} OneHitKnockOut;

static void MiscTab() 
{ 
    if (ImGui::Checkbox("Enabled", &Enabled)) 
    {
        Settings::SetSetting({ "Misc", "Enabled" }, Enabled);
    }

    if (!Enabled) 
    {
        return;
    }

    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (!pawn || !controller) 
    {
        return;
    }

    ImGui::Separator(5.0f);

    if (ImGui::Checkbox("Auto Roll", &AutoRollEnabled)) 
    {
        Settings::SetSetting({ "Misc", "AutoRollEnabled" }, AutoRollEnabled);

        if (!AutoRollEnabled) 
        {
            pawn->RollTriggerTime = 0.0f;
        }
    }
    ImGui::HelpMarker("If enabled, it will automatically roll every time for you");

    if (ImGui::Checkbox("No Auto Lockon", &AutoLockDisabled))
    {
        Settings::SetSetting({ "Misc", "AutoLockDisabled" }, AutoLockDisabled);
    }
    ImGui::HelpMarker("Disables the camera lock on when getting too close to an AI");

    // Color Scale
    {
        if (ImGui::Checkbox("Customize Color Scale", &CustomColorScaleEnabled))
        {
            Settings::SetSetting({ "Misc", "ColorScale", "Enabled" }, CustomColorScaleEnabled);

            if (!CustomColorScaleEnabled && controller->PlayerCamera)
            {
                controller->PlayerCamera->ColorScale = Classes::FVector{ 1.0f, 1.0f, 1.0f };
            }
        }

        if (CustomColorScaleEnabled)
        {
            ImGui::ColorEdit3("Color Scale", &CustomColorScaleValues.X, ImGuiColorEditFlags_Float);
            ImGui::HelpMarker("Default value for all is 1.0 (white)");

            if (ImGui::Button("Save##ColorScaleSave"))
            {
                Settings::SetSetting({ "Misc", "ColorScale", "Values" }, FVectorToJson(CustomColorScaleValues));
            }
        }
    }

    // Tunnel Vision
    {
        if (ImGui::Checkbox("Tunnel Vision", &TunnelVisionEnabled))
        {
            Settings::SetSetting({ "Misc", "TunnelVision", "Enabled" }, TunnelVisionEnabled);
        }

        if (TunnelVisionEnabled)
        {
            if (ImGui::Checkbox("Tunnel Vision Inverted", &TunnelVisionInverted))
            {
                Settings::SetSetting({ "Misc", "TunnelVision", "Inverted" }, TunnelVisionInverted);
            }

            ImGui::Text("Tunnel Vision Size");
            if (ImGui::InputFloat2("##TunnelVision-Size", &TunnelVisionSize.x, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue))
            {
                Settings::SetSetting({ "Misc", "TunnelVision", "Width" }, TunnelVisionSize.x);
                Settings::SetSetting({ "Misc", "TunnelVision", "Height" }, TunnelVisionSize.y);
            }
            ImGui::HelpMarker("Customize the width and height. If you wanted cinematic bars, the width needs to be greater than half your "
                "current width. For the height, it needs to be less than half the height. Press enter to save the width and height.\n\nDefault value for both is 128");
        }
    }

    if (ImGui::Checkbox("No Consequtive Wallruns Limit", &ConsequtiveWallrunsLimitRemoved)) 
    {
        Settings::SetSetting({ "Misc", "ConsequtiveWallrunsLimitRemoved" }, ConsequtiveWallrunsLimitRemoved);
    }
    ImGui::HelpMarker("Removes the consequtive wallrun limit");

    // Permanent Reaction Time
    {
        if (!PermanentGameSpeedEnabled)
        {
            if (ImGui::Checkbox("Permanent Reaction Time", &PermanentReactionTimeEnabled))
            {
                Settings::SetSetting({ "Misc", "PermanentReactionTimeEnabled" }, PermanentReactionTimeEnabled);
                Settings::SetSetting({ "Misc", "PermanentGameSpeedEnabled" }, PermanentGameSpeedEnabled = false);

                pawn->WorldInfo->TimeDilation = 1.0f;
            }
            ImGui::HelpMarker("Once reactiontime is activated, it will be on forever until faith die, level changed, or deactivated. Can't be used in timetrials");
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::Checkbox("Permanent Reaction Time", &PermanentReactionTimeEnabled);
            ImGui::EndDisabled();
        }
    }

    // Permanent Game Speed
    {
        if (!PermanentReactionTimeEnabled)
        {
            if (ImGui::Checkbox("Permanent Game Speed", &PermanentGameSpeedEnabled))
            {
                Settings::SetSetting({ "Misc", "PermanentGameSpeedEnabled" }, PermanentGameSpeedEnabled);
                Settings::SetSetting({ "Misc", "PermanentReactionTimeEnabled" }, PermanentReactionTimeEnabled = false);

                if (!PermanentGameSpeedEnabled)
                {
                    pawn->WorldInfo->TimeDilation = 1.0f;
                }
            }
            ImGui::HelpMarker("Is always active, it will be forever on until deactivated");

            if (PermanentGameSpeedEnabled)
            {
                if (ImGui::InputFloat("Game Speed", &PermanentGameSpeed, 0.1f, 0.5f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    Settings::SetSetting({ "Misc", "PermanentGameSpeed" }, PermanentGameSpeed = max(0.1f, PermanentGameSpeed));
                }
            }
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::Checkbox("Permanent Game Speed", &PermanentGameSpeedEnabled);
            ImGui::EndDisabled();
        }
    }

    ImGui::SeparatorText("Challenges");
    {
        if (ImGui::Checkbox("No Wallrun Challenge", &NoWallrunChallenge)) 
        {
            Settings::SetSetting({ "Misc", "Challenges", "NoWallrun" }, NoWallrunChallenge);
        }
        ImGui::HelpMarker("If you wallrun you start from new game");

        if (ImGui::Checkbox("No Wallclimb Challenge", &NoWallclimbChallenge)) 
        {
            Settings::SetSetting({ "Misc", "Challenges", "NoWallclimb"}, NoWallclimbChallenge);
        }
        ImGui::HelpMarker("If you wallclimb you start from new game");

        // No Health Regeneration
        {
            if (!OneHitKnockOut.Enabled) 
            {
                if (ImGui::Checkbox("No Health Regeneration", &NoHealthRegenerationEnabled)) 
                {
                    Settings::SetSetting({ "Misc", "Challenges", "NoHealthRegeneration" }, NoHealthRegenerationEnabled);
            
                    OneHitKnockOut.Health = 100;
                    pawn->Health = pawn->MaxHealth;
                }

                ImGui::HelpMarker("Health regeneration is turned off. If you have 100 health and take 15 damage, you'll have 85 health for the rest of the game. If you reach 0 health, it will start a new game for you");
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Checkbox("No Health Regeneration", &NoHealthRegenerationEnabled);
                ImGui::HelpMarker("One Hit KO is on and this can't be enabled. It is the same except in One Hit KO, there's no health regeneration");
                ImGui::EndDisabled();
            }
        }

        // One Hit Knock Down
        {
            if (!NoHealthRegenerationEnabled) 
            {
                if (ImGui::Checkbox("One Hit KO", &OneHitKnockOut.Enabled))
                {
                    Settings::SetSetting({ "Misc", "OneHitKnockOut", "Enabled" }, OneHitKnockOut.Enabled);

                    OneHitKnockOut.Health = 100;
                    pawn->Health = pawn->MaxHealth;
                }

                // Ohko Type Settings
                if (OneHitKnockOut.Enabled)
                {
                    ImGui::DummyVertical(5.0f);
                    ImGui::Text("Type");

                    if (ImGui::RadioButton("Normal", OneHitKnockOut.Type == EOhko::Normal))
                    {
                        Settings::SetSetting({ "Misc", "OneHitKnockOut", "Type" }, OneHitKnockOut.Type = EOhko::Normal);
                    }
                    ImGui::HelpMarker("If you take any damage at all, you'll respawn at the last checkpoint");

                    if (ImGui::RadioButton("Extreme", OneHitKnockOut.Type == EOhko::Extreme))
                    {
                        Settings::SetSetting({ "Misc", "OneHitKnockOut", "Type" }, OneHitKnockOut.Type = EOhko::Extreme);
                    }
                    ImGui::HelpMarker("If you take any damage at all, it will instantly start a new game");
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Checkbox("One Hit KO", &OneHitKnockOut.Enabled);
                ImGui::EndDisabled();
            }
        }
    }
}

static void StartNewGame() 
{
    const auto gameInfo = static_cast<Classes::ATdGameInfo *>(Engine::GetWorld()->Game);
    gameInfo->TdGameData->StartNewGameWithTutorial(true);

    OneHitKnockOut.Health = 100;
}

static void OnTick(float deltaTime) 
{
    if (!Enabled) 
    {
        return;
    }

    auto world = Engine::GetWorld();
    if (!world) 
    {
        return;
    }

    if (!PermanentReactionTimeEnabled && PermanentGameSpeedEnabled) 
    {
        world->TimeDilation = PermanentGameSpeed;
    }

    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();
    if (!pawn || !controller) 
    {
        return;
    }

    if (!PermanentReactionTimeEnabled && PermanentGameSpeedEnabled) 
    {
        controller->ReactionTimeEnergy = 0.0f;
    }

    if (PermanentReactionTimeEnabled && !PermanentGameSpeedEnabled) 
    {
        if (controller->bReactionTime && controller->ReactionTimeEnergy <= 50.0f) 
        {
            controller->ReactionTimeEnergy = 50.0f;
        }
    }

    if (LevelName.empty()) 
    {
        
        LevelName = GetLowercasedLevelName(pawn->WorldInfo->GetMapName(false).c_str());
        std::transform(LevelName.begin(), LevelName.end(),LevelName.begin(), [](char c) 
        {
            return tolower(c); 
        });
    }

    if (OneHitKnockOut.Enabled && LevelName != Map_MainMenu && (pawn->EnterFallingHeight != -1e30f && pawn->Health != pawn->MaxHealth))
    {
        if (OneHitKnockOut.Type == EOhko::Normal)
        {
            pawn->RegenerateDelay = 5;
            pawn->RegenerateHealthPerSecond = 25;

            if (pawn->Health != pawn->MaxHealth) 
            {
                pawn->Suicide();
            }
        } 
        else if (OneHitKnockOut.Type == EOhko::Extreme)
        {
            if (pawn->Health != pawn->MaxHealth) 
            {
                StartNewGame();
            }
        }
    }

    if (NoHealthRegenerationEnabled && LevelName != Map_MainMenu && (pawn->EnterFallingHeight != -1e30f && pawn->Health != pawn->MaxHealth)) 
    {
        pawn->RegenerateDelay = 0;
        pawn->RegenerateHealthPerSecond = 0;

        if (OneHitKnockOut.Health != pawn->Health)
        {
            if (pawn->Health == pawn->MaxHealth) 
            {
                pawn->Health = OneHitKnockOut.Health;
            } 
            else if (pawn->Health <= 0) 
            {
                StartNewGame();
            } 
            else 
            {
                if (OneHitKnockOut.Health > pawn->Health)
                {
                    OneHitKnockOut.Health = pawn->Health;
                }

                if (OneHitKnockOut.Health <= 0)
                {
                    StartNewGame();
                }
            }
        }
    }

    if (ConsequtiveWallrunsLimitRemoved) 
    {
        if (pawn->MovementState == Classes::EMovement::MOVE_WallRunningLeft || pawn->MovementState == Classes::EMovement::MOVE_WallRunningRight) 
        {
            const auto wallrun = static_cast<Classes::UTdMove_WallRun *>(pawn->Moves[static_cast<size_t>(Classes::EMovement::MOVE_WallRunningRight)]);

            if (wallrun && wallrun->ConsequtiveWallruns > 0) 
            {
                wallrun->ConsequtiveWallruns = 0;
            }
        }
    }

    if (AutoLockDisabled)
    {
        if (controller->TargetingPawn)
        {
            controller->TargetingPawn->SoftLockStrength = 0;
        }
    }

    if (AutoRollEnabled) 
    {
        pawn->RollTriggerTime = 1e30f;
    }

    if (NoWallrunChallenge) 
    {
        if (pawn->MovementState == Classes::EMovement::MOVE_WallRunningLeft || pawn->MovementState == Classes::EMovement::MOVE_WallRunningRight) 
        {
            StartNewGame();
        }
    }

    if (NoWallclimbChallenge) 
    {
        if (pawn->MovementState == Classes::EMovement::MOVE_WallClimbing) 
        {
            StartNewGame();
        }
    }

    if (CustomColorScaleEnabled)
    {
        if (controller->PlayerCamera)
        {
            controller->PlayerCamera->ColorScale = CustomColorScaleValues;
        }
    }
}

static void OnRender(IDirect3DDevice9* device)
{
    if (!Enabled)
    {
        return;
    }

    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (!pawn && !controller || LevelName == Map_MainMenu)
    {
        return;
    }

    if (TunnelVisionEnabled)
    {
        auto window = ImGui::BeginRawScene("##TunnelVision-Scene");
        auto displaySize = ImGui::GetMainViewport()->Size;
        const float width = TunnelVisionSize.x;
        const float height = TunnelVisionSize.y;

        if (!TunnelVisionInverted)
        {
            window->DrawList->AddRectFilled({ 0, 0 }, { displaySize.x / 2 + width, displaySize.y / 2 - height }, IM_COL32_BLACK);
            window->DrawList->AddRectFilled({ displaySize.x / 2 + width, 0 }, { displaySize.x, displaySize.y / 2 + height }, IM_COL32_BLACK);
            window->DrawList->AddRectFilled({ displaySize.x / 2 - width, displaySize.y / 2 + height }, { displaySize.x, displaySize.y }, IM_COL32_BLACK);
            window->DrawList->AddRectFilled({ 0, displaySize.y / 2 - height }, { displaySize.x / 2 - width, displaySize.y }, IM_COL32_BLACK);
        }
        else
        {
            window->DrawList->AddRectFilled({ displaySize.x / 2 - width, displaySize.y / 2 - height }, { displaySize.x / 2 + width, displaySize.y / 2 + height }, IM_COL32_BLACK);
        }

        ImGui::EndRawScene();
    }
}

bool Misc::Initialize() 
{
    Enabled = Settings::GetSetting({ "Misc", "Enabled" }, false);

    ConsequtiveWallrunsLimitRemoved = Settings::GetSetting({ "Misc", "ConsequtiveWallrunsLimitRemoved" }, false);
    AutoRollEnabled = Settings::GetSetting({ "Misc", "AutoRollEnabled" }, false);
    AutoLockDisabled = Settings::GetSetting({ "Misc", "AutoLockDisabled" }, false);
    PermanentReactionTimeEnabled = Settings::GetSetting({ "Misc", "PermanentReactionTimeEnabled" }, false);
    PermanentGameSpeedEnabled = Settings::GetSetting({ "Misc", "PermanentGameSpeedEnabled" }, false);
    PermanentGameSpeed = Settings::GetSetting({ "Misc", "PermanentGameSpeed" }, 0.25f);
    NoWallrunChallenge = Settings::GetSetting({ "Misc", "Challenges", "NoWallrun" }, false);
    NoWallclimbChallenge = Settings::GetSetting({ "Misc", "Challenges", "NoWallclimb" }, false);
    NoHealthRegenerationEnabled = Settings::GetSetting({ "Misc", "Challenges", "NoHealthRegeneration" }, false);

    OneHitKnockOut.Enabled = Settings::GetSetting({ "Misc", "OneHitKnockOut", "Enabled" }, false);
    OneHitKnockOut.Type = Settings::GetSetting({ "Misc", "OneHitKnockOut", "Type" }, EOhko::Normal).get<EOhko>();

    TunnelVisionEnabled = Settings::GetSetting({ "Misc", "TunnelVision", "Enabled" }, false);
    TunnelVisionInverted = Settings::GetSetting({ "Misc", "TunnelVision", "Inverted" }, false);
    TunnelVisionSize.x = Settings::GetSetting({ "Misc", "TunnelVision", "Width" }, 128.0f);
    TunnelVisionSize.y = Settings::GetSetting({ "Misc", "TunnelVision", "Height" }, 128.0f);

    CustomColorScaleEnabled = Settings::GetSetting({ "Misc", "ColorScale", "Enabled" }, false);
    CustomColorScaleValues = JsonToFVector(Settings::GetSetting({ "Misc", "ColorScale", "Values" }, FVectorToJson(Classes::FVector{1.0f, 1.0f, 1.0f})));

    Menu::AddTab("Misc", MiscTab);
    Engine::OnTick(OnTick);
    Engine::OnRenderScene(OnRender);

    Engine::OnPostLevelLoad([](const wchar_t *newLevelName) 
    {
        LevelName = GetLowercasedLevelName(newLevelName);
        
    });

    return true;
}

std::string Misc::GetName() 
{ 
    return "Misc"; 
}
