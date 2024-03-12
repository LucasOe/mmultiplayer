#include <codecvt>

#include "../engine.h"
#include "../imgui/imgui.h"
#include "../menu.h"
#include "../settings.h"
#include "../util.h"
#include "misc.h"

static bool Enabled = false;
static std::string LevelName;

static bool ConsequtiveWallrunsLimitRemoved = false;
static bool AutorollEnabled = false;
static bool PermanentReactionTimeEnabled = false;
static bool PermanentGameSpeedEnabled = false;
static float PermanentGameSpeed = 0.25f;
static bool NoWallrunChallenge = false;
static bool NoWallclimbChallenge = false;
static bool NoHealthRegenerationEnabled = false;

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

    // Auto Roll
    if (ImGui::Checkbox("Auto Roll", &AutorollEnabled)) 
    {
        Settings::SetSetting({ "Misc", "AutorollEnabled" }, AutorollEnabled);

        if (!AutorollEnabled) 
        {
            pawn->RollTriggerTime = 0.0f;
        }
    }
    ImGui::HelpMarker("If enabled, it will automatically roll every time for you");

    // Consequtive Wallruns
    if (ImGui::Checkbox("No Consequtive Wallruns Limit", &ConsequtiveWallrunsLimitRemoved)) 
    {
        Settings::SetSetting({ "Misc", "ConsequtiveWallrunsLimitRemoved" }, ConsequtiveWallrunsLimitRemoved);
    }
    ImGui::HelpMarker("Removes the limit so you can wallrun, jump to another wall and wallrun ");

    // No Wallrun Challenge
    if (ImGui::Checkbox("No Wallrun Challenge", &NoWallrunChallenge)) 
    {
        Settings::SetSetting({ "Misc", "Challenges", "NoWallrun" }, NoWallrunChallenge);
    }
    ImGui::HelpMarker("If you wallrun you start from new game");

    // No Wallclimb Challenge
    if (ImGui::Checkbox("No Wallclimb Challenge", &NoWallclimbChallenge)) 
    {
        Settings::SetSetting({ "Misc", "Challenges", "NoWallclimb"}, NoWallclimbChallenge);
    }
    ImGui::HelpMarker("If you wallclimb you start from new game");

    // No Health Regeneration
    if (!OneHitKnockOut.Enabled) 
    {
        if (ImGui::Checkbox("No Health Regeneration", &NoHealthRegenerationEnabled)) 
        {
            Settings::SetSetting({ "Misc", "Challenges", "NoHealthRegeneration" }, NoHealthRegenerationEnabled);
            
            OneHitKnockOut.Health = 100;
            pawn->Health = pawn->MaxHealth;
        }

        ImGui::HelpMarker("Health regeneration is turned off. If you have 100 health and take 15 damage, you'll have 85 health for the rest of the game.\nIf you reach 0 health, it will start a new game for you");
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Checkbox("No Health Regeneration", &NoHealthRegenerationEnabled);
        ImGui::HelpMarker("One Hit KO is on and this can't be enabled. It is the same except in One Hit KO, there's no health regeneration");
        ImGui::EndDisabled();
    }

    // Permanent Reaction Time
    if (ImGui::Checkbox("Permanent Reaction Time", &PermanentReactionTimeEnabled)) 
    {
        Settings::SetSetting({ "Misc", "PermanentReactionTimeEnabled" }, PermanentReactionTimeEnabled);
        Settings::SetSetting({ "Misc", "PermanentGameSpeedEnabled" }, PermanentGameSpeedEnabled = false);

        pawn->WorldInfo->TimeDilation = 1.0f;
    }
    ImGui::HelpMarker("Once reactiontime is activated, it will be on forever until faith die, level changed, or deactivated.\nCan't be used in timetrials");

    // Permanent Game Speed
    if (ImGui::Checkbox("Permanent Game Speed", &PermanentGameSpeedEnabled)) 
    {
        Settings::SetSetting({ "Misc", "PermanentGameSpeedEnabled" }, PermanentGameSpeedEnabled);
        Settings::SetSetting({ "Misc", "PermanentReactionTimeEnabled" }, PermanentReactionTimeEnabled = false);

        if (!PermanentGameSpeedEnabled) 
        {
            pawn->WorldInfo->TimeDilation = 1.0f; 
        }
    }
    ImGui::HelpMarker("Is always active, it will be forever on until deactivated. Can be used in timetrials");

    if (PermanentGameSpeedEnabled) 
    {
        if (ImGui::InputFloat("Game Speed", &PermanentGameSpeed, 0.1f, 0.5f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) 
        {
            Settings::SetSetting({ "Misc", "PermanentGameSpeed" }, PermanentGameSpeed = max(0.1f, PermanentGameSpeed));
        }
    }

    // One Hit Knock Down
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
        LevelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(pawn->WorldInfo->GetMapName(false).c_str());

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

    if (AutorollEnabled) 
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
}

bool Misc::Initialize() 
{
    Enabled = Settings::GetSetting({ "Misc", "Enabled" }, false);

    ConsequtiveWallrunsLimitRemoved = Settings::GetSetting({ "Misc", "ConsequtiveWallrunsLimitRemoved" }, false);
    AutorollEnabled = Settings::GetSetting({ "Misc", "AutorollEnabled" }, false);
    PermanentReactionTimeEnabled = Settings::GetSetting({ "Misc", "PermanentReactionTimeEnabled" }, false);
    PermanentGameSpeedEnabled = Settings::GetSetting({ "Misc", "PermanentGameSpeedEnabled" }, false);
    PermanentGameSpeed = Settings::GetSetting({ "Misc", "PermanentGameSpeed" }, 0.25f);
    NoWallrunChallenge = Settings::GetSetting({ "Misc", "Challenges", "NoWallrun" }, false);
    NoWallclimbChallenge = Settings::GetSetting({ "Misc", "Challenges", "NoWallclimb" }, false);
    NoHealthRegenerationEnabled = Settings::GetSetting({ "Misc", "Challenges", "NoHealthRegeneration" }, false);

    OneHitKnockOut.Enabled = Settings::GetSetting({ "Misc", "OneHitKnockOut", "Enabled" }, false);
    OneHitKnockOut.Type = Settings::GetSetting({ "Misc", "OneHitKnockOut", "Type" }, EOhko::Normal).get<EOhko>();

    Menu::AddTab("Misc", MiscTab);
    Engine::OnTick(OnTick);

    Engine::OnPostLevelLoad([](const wchar_t *newLevelName) 
    {
        LevelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(newLevelName);
        std::transform(LevelName.begin(), LevelName.end(), LevelName.begin(), [](char c) { return tolower(c); });
    });

    return true;
}

std::string Misc::GetName() 
{ 
    return "Misc"; 
}
