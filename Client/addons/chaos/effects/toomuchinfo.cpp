#pragma once

#include "../effect.h"

class TooMuchInfo : public Effect
{
public:
    TooMuchInfo(const std::string& name)
    {
        Name = name;
        DisplayName = name;
        DurationType = EDuration::Long;
    }

    void Start() override {}

    void Tick(const float deltaTime) override {}

    void Render(IDirect3DDevice9* device) override 
    {
        const auto pawn = Engine::GetPlayerPawn();
        const auto controller = Engine::GetPlayerController();

        if (!pawn || !controller)
        {
            return;
        }

        RenderPlayerInfo(pawn); 
        RenderPlayerCameraInfo(static_cast<Classes::ATdPlayerCamera*>(controller->PlayerCamera));
        RenderControllerInfo(controller);
        RenderWeaponInfo(pawn->MyWeapon);
        RenderWorldInfo(pawn->WorldInfo);
        RenderPlayerInputInfo(GetTdPlayerInput());
    }

    bool Shutdown() override
    {
        return true;
    }

    std::string GetType() const override
    {
        return "UI";
    }

private:
    void WindowBegin(const char* title, const char* name, ImVec2 pos)
    {
        const auto& io = ImGui::GetIO();
        const float scaleX = io.DisplaySize.x / 1920.0f;
        const float scaleY = io.DisplaySize.y / 1080.0f;
        const float scale = (scaleX < scaleY) ? scaleX : scaleY;

        pos *= scale;

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.75f));
        ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text(title);
        ImGui::Separator(1.0f);
    }

    void WindowEnd() 
    {
        ImGui::End();
        ImGui::PopStyleColor();
    }

    void RenderPlayerInfo(Classes::ATdPawn* pawn)
    {
        WindowBegin("ATdPlayerPawn", "##RenderPlayerInfo", ImVec2(420, 256));

        ImGui::Text("Location: %.2f %.2f %.2f", pawn->Location.X, pawn->Location.Y, pawn->Location.Z);
        ImGui::Text("Velocity: %.2f %.2f %.2f", pawn->Velocity.X, pawn->Velocity.Y, pawn->Velocity.Z);
        ImGui::Text("Acceleration: %.2f %.2f %.2f", pawn->Acceleration.X, pawn->Acceleration.Y, pawn->Acceleration.Z);
        ImGui::Text("bIsFemale: %d", pawn->bIsFemale);
        ImGui::Text("bCanUse: %d", pawn->bCanUse);
        ImGui::Text("Health: %d", pawn->Health);
        ImGui::Text("bIsWallWalking: %d", pawn->bIsWallWalking);
        ImGui::Text("bAllowMoveChange: %d", pawn->bAllowMoveChange);
        ImGui::Text("bCharacterInhaling: %d", pawn->bCharacterInhaling);
        ImGui::Text("VelocityMagnitude: %.2f", pawn->VelocityMagnitude);
        ImGui::Text("VelocityDir: %.2f %.2f %.2f", pawn->VelocityDir.X, pawn->VelocityDir.Y, pawn->VelocityDir.Z);
        ImGui::Text("LastDamageTaken: %d", pawn->LastDamageTaken);
        ImGui::Text("LastJumpLocation: %.2f %.2f %.2f", pawn->LastJumpLocation.X, pawn->LastJumpLocation.Y, pawn->LastJumpLocation.Z);

        WindowEnd();
    }

    void RenderPlayerCameraInfo(Classes::ATdPlayerCamera* camera)
    {
        WindowBegin("ATdPlayerCamera", "##RenderPlayerCameraInfo", ImVec2(1500, 200));

        if (!camera)
        {
            ImGui::Text("nullptr");
            WindowEnd();
            return;
        }

        const auto controller = Engine::GetPlayerController();
        ImGui::Text("bLockedFOV: %d", camera->bLockedFOV);
        ImGui::Text("LockedFOV: %.2f", camera->LockedFOV);
        ImGui::Text("FOVAngle: %.2f", controller->FOVAngle);
        ImGui::Text("DesiredFOV: %.2f", controller->DesiredFOV);
        ImGui::Text("DefaultFOV: %.2f", controller->DefaultFOV);
        ImGui::Text("ColorScale: %.2f %.2f %.2f", camera->ColorScale.X, camera->ColorScale.Y, camera->ColorScale.Z);

        WindowEnd();
    }

    void RenderControllerInfo(Classes::ATdPlayerController* controller)
    {
        WindowBegin("ATdPlayerController", "##RenderControllerInfo", ImVec2(420, 666));

        ImGui::Text("Rotation: %d %d %d", controller->Rotation.Pitch, controller->Rotation.Yaw % 65536, controller->Rotation.Roll);
        ImGui::Text("TimePressedJump: %.2f", controller->TimePressedJump);
        ImGui::Text("bIgnoreMoveInput: %d", controller->bIgnoreMoveInput);
        ImGui::Text("bIgnoreLookInput: %d", controller->bIgnoreLookInput);
        ImGui::Text("bIgnoreButtonInput: %d", controller->bIgnoreButtonInput);
        ImGui::Text("bCinematicMode: %d", controller->bCinematicMode);
        ImGui::Text("bDuck: %d", controller->bDuck);
        ImGui::Text("bReactionTime: %d", controller->bReactionTime);
        ImGui::Text("ReactionTimeEnergy: %.2f", controller->ReactionTimeEnergy);
        ImGui::Text("ReactionTimeEnergyBuildRate: %.3f", controller->ReactionTimeEnergyBuildRate);
        ImGui::Text("MouseX: %.0f", controller->MouseX);
        ImGui::Text("MouseY: %.0f", controller->MouseY);

        WindowEnd();
    }

    void RenderWeaponInfo(Classes::ATdWeapon* weapon)
    {
        WindowBegin("ATdWeapon", "##RenderWeaponInfo", ImVec2(1500, 475));

        if (!weapon)
        {
            ImGui::Text("nullptr");
            WindowEnd();
            return;
        }

        ImGui::Text("Name: %s", weapon->Name.GetName().c_str());
        ImGui::Text("EquipTime: %.2f", weapon->EquipTime);
        ImGui::Text("PutDownTime: %.2f", weapon->PutDownTime);
        ImGui::Text("WeaponRange: %.0f", weapon->WeaponRange);
        ImGui::Text("bCanThrow: %d", weapon->bCanThrow);
        ImGui::Text("bCanZoom: %d", weapon->bCanZoom);
        ImGui::Text("MaxAmmo: %d", weapon->MaxAmmo);
        ImGui::Text("AmmoCount: %d", weapon->AmmoCount);
        ImGui::Text("FallOffDistance: %.2f", weapon->FallOffDistance);
        ImGui::Text("DeathAnimType: %d", weapon->DeathAnimType);
        ImGui::Text("RecoilAmount: %.2f", weapon->RecoilAmount);
        ImGui::Text("RecoilRecoverTime: %.2f", weapon->RecoilRecoverTime);
        ImGui::Text("MinRecoil: %.2f", weapon->MinRecoil);
        ImGui::Text("MaxRecoil: %.2f", weapon->MaxRecoil);
        ImGui::Text("KickbackAmount: %.2f", weapon->KickbackAmount);

        // Only the sniper has extra stuff that could be display. Shotguns have "PulletCount" but isn't used
        if (GetObjectName(weapon) == "TdWeapon_Sniper_BarretM95")
        {
            const auto sniper = static_cast<Classes::ATdWeapon_Sniper_BarretM95*>(weapon);
            ImGui::Text("bZoomed: %d", sniper->bZoomed);
            ImGui::Text("bIsZooming: %d", sniper->bIsZooming);
            ImGui::Text("ZoomFOV: %.2f", sniper->ZoomFOV);
            ImGui::Text("ZoomRate: %.2f", sniper->ZoomRate);
            ImGui::Text("ZoomDelay: %.2f", sniper->ZoomDelay);
            ImGui::Text("AdditionalUnzoomedSpread: %.2f", sniper->AdditionalUnzoomedSpread);
        }

        ImGui::Text("Spread[0]: %f", weapon->Spread[0]);

        WindowEnd();
    }

    void RenderWorldInfo(Classes::AWorldInfo* world)
    {
        WindowBegin("AWorldInfo", "##RenderWorldInfo", ImVec2(800, 800));

        if (!world)
        {
            ImGui::Text("nullptr");
            WindowEnd();
            return;
        }

        ImGui::Text("TimeDilation: %.2f", world->TimeDilation);
        ImGui::Text("TimeSeconds: %.2f", world->TimeSeconds);
        ImGui::Text("RealTimeSeconds: %.2f", world->RealTimeSeconds);
        ImGui::Text("AudioTimeSeconds: %.2f", world->AudioTimeSeconds);
        ImGui::Text("WorldGravityZ: %.2f", world->WorldGravityZ);

        WindowEnd();
    }
    
    void RenderPlayerInputInfo(Classes::UTdPlayerInput* input)
    {
        WindowBegin("UTdPlayerInput", "##RenderPlayerInputInfo", ImVec2(1200, 700));

        if (!input)
        {
            ImGui::Text("nullptr");
            WindowEnd();
            return;
        }

        ImGui::Text("bUsingGamepad: %d", input->bUsingGamepad);
        ImGui::Text("bInvertMouse: %d", input->bInvertMouse);
        ImGui::Text("bEnableMouseSmoothing: %d", input->bEnableMouseSmoothing);
        ImGui::Text("MaxSensitivityMultiplier: %.2f", input->MaxSensitivityMultiplier);
        ImGui::Text("MinSensitivityMultiplier: %.2f", input->MinSensitivityMultiplier);
        ImGui::Text("SensitivityMultiplier: %.2f", input->SensitivityMultiplier);
        ImGui::Text("MouseSensitivity: %.2f", input->MouseSensitivity);
        ImGui::Text("PressedKeys.Num(): %d", input->PressedKeys.Num());

        for (size_t i = 0; i < input->PressedKeys.Num(); i++)
        {
            ImGui::Text("[%d] %s", i, input->PressedKeys[i].GetName().c_str());
        }

        WindowEnd();
    }
};

REGISTER_EFFECT(TooMuchInfo, "Too Much Info");
