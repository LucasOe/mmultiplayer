#include <algorithm>

#include "../imgui/imgui.h"
#include "../menu.h"
#include "../pattern.h"
#include "../settings.h"
#include "dolly.h"
#include <fstream>
#include <shlobj.h>

static auto recording = false, playing = false, cameraView = false, pathDisplay = true;

static bool ToggleResetKeybinds = false;

static int duration = 0, frame = 0;
static std::vector<Dolly::Marker> markers;

static auto character = Engine::Character::Faith;
static Dolly::Recording currentRecording;
static std::vector<Dolly::Recording> recordings;

static void *forceRollPatch = nullptr;
static byte forceRollPatchOriginal[6];

static bool hideQueued = false;

static unsigned long oldProtect;

static int MarkerKeybind = 0;
static int RecordKeybind = 0;
static int JumpFramesKeybind = 0;
static int StartStopKeybind = 0;

static int FOVPlusKeybind = 0;
static int FOVMinusKeybind = 0;
static int RollPlusKeybind = 0;
static int RollMinusKeybind = 0;

static int FrameJumpAmount = 100;

static auto forceRoll = false;

std::string GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return std::string(path) + "\\MMultiplayer\\";
    }
    return "";
}

void Dolly::ForceRoll(bool force) {
    if (force) {
        memcpy(forceRollPatch, "\x90\x90\x90\x90\x90\x90", 6);
    } else {
        memcpy(forceRollPatch, forceRollPatchOriginal, 6);
    }
}

void SaveMarkers();
void LoadMarkers();
void SaveRecordings();
void LoadRecordings();

static void ProcessHotkey(const char *label, int *key, const std::vector<std::string> settings, const int defaultValue) {
    if (settings.empty()) {
        return;
    }

    if (ImGui::Hotkey(label, key)) {
        Settings::SetSetting(settings, *key);
    }

    if (ToggleResetKeybinds) {
        ImGui::SameLine();

        if (ImGui::Button(("Reset##" + settings.back()).c_str())) {
            Settings::SetSetting(settings, *key = defaultValue);
        }
    }
}

static Classes::FRotator VectorToRotator(Classes::FVector vector) {
    auto convert = [](float r) {
        return static_cast<unsigned int>((fmodf(r, 360.0f) / 360.0f) * 0x10000);
    };

    return Classes::FRotator{convert(vector.X), convert(vector.Y), convert(vector.Z)};
}

static Classes::FVector RotatorToVector(Classes::FRotator rotator) {
    auto convert = [](unsigned int r) {
        return (static_cast<float>(r % 0x10000) / static_cast<float>(0x10000)) * 360.0f;
    };

    return Classes::FVector{convert(rotator.Pitch), convert(rotator.Yaw), convert(rotator.Roll)};
}

static float Interpolate(float x0, float x1, float y0, float y1, float m0, float m1, float x) {
    auto t = (x - x0) / (x1 - x0);
    auto t2 = t * t;
    auto t3 = t2 * t;

    auto h00 = (2 * t3) - (3 * t2) + 1;
    auto h10 = t3 - (2 * t2) + t;
    auto h01 = (-2 * t3) + (3 * t2);
    auto h11 = t3 - t2;

    auto domain = x1 - x0;

    return (h00 * y0) + (h10 * domain * m0) + (h01 * y1) + (h11 * domain * m1);
}

static inline float GetMarkerField(int index, int fieldOffset) {
    return *reinterpret_cast<float *>(reinterpret_cast<byte *>(&markers[index]) + fieldOffset);
}

// Must have more than 1 marker
static float Slope(int index, int fieldOffset) {
    if (index == 0) {
        return (GetMarkerField(index + 1, fieldOffset) - GetMarkerField(index, fieldOffset)) /
               static_cast<float>(markers[index + 1].Frame - markers[index].Frame);
    } else if (index == markers.size() - 1) {
        return (GetMarkerField(index, fieldOffset) - GetMarkerField(index - 1, fieldOffset)) /
               static_cast<float>(markers[index].Frame - markers[index - 1].Frame);
    }

    return 0.5f * (((GetMarkerField(index + 1, fieldOffset) - GetMarkerField(index, fieldOffset)) /
                    static_cast<float>(markers[index + 1].Frame - markers[index].Frame)) +
                   (GetMarkerField(index, fieldOffset) - GetMarkerField(index - 1, fieldOffset)) /
                       static_cast<float>(markers[index].Frame - markers[index - 1].Frame));
}

static void FixTimeline() {
    duration = 0;

    for (auto &m : markers) {
        // Normalize the rotations
        m.Rotation = RotatorToVector(VectorToRotator(m.Rotation));

        duration = max(m.Frame, duration);
    }

    for (auto &r : recordings) {
        duration = max(r.StartFrame + static_cast<int>(r.Frames.size()) - 1, duration);
    }

    std::sort(markers.begin(), markers.end(),
              [](const Dolly::Marker &a, const Dolly::Marker &b) { return a.Frame < b.Frame; });

    if (markers.size() > 0) {
        auto &first = markers[0].Rotation;
        first.X += 360.0f;
        first.Y += 360.0f;
        first.Z += 360.0f;

        for (auto i = 1UL; i < markers.size(); ++i) {
            auto &prev = markers[i - 1].Rotation;
            auto &curr = markers[i].Rotation;

            // Make each turn take the shortest distance
            auto shorten = [](float prev, float &curr) {
                while (curr < prev) {
                    curr += 360.0f;
                }

                if (fabsf((curr - 360.0f) - prev) < fabsf(curr - prev)) {
                    curr -= 360.0f;
                }
            };

            shorten(prev.X, curr.X);
            shorten(prev.Y, curr.Y);
            shorten(prev.Z, curr.Z);
        }
    }
}

static void ShiftTimeline(int amount) {
    frame += amount;

    for (auto &m : markers) {
        m.Frame += amount;
    }

    for (auto &r : recordings) {
        r.StartFrame += amount;
    }
}

static void FixPlayer() {
    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    Dolly dolly;

    if (!pawn || !controller) {
        dolly.ForceRoll(false);
        return;
    }

    auto hide = playing || cameraView;

    pawn->bCollideWorld = !hide;
    pawn->Physics = hide ? Classes::EPhysics::PHYS_None : Classes::EPhysics::PHYS_Walking;
    controller->bCanBeDamaged = !hide;
    controller->PlayerCamera->SetFOV(controller->DefaultFOV);
    hideQueued = true;

    if (hide) {
        dolly.ForceRoll(true);
    } else {
        for (auto &r : recordings) {
            if (r.Actor) {
                r.Actor->Location = {0};
            }
        }

        pawn->EnterFallingHeight = -1e30f;
        dolly.ForceRoll(false);
    }
}

static void AddMarker() {
    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();
    if (pawn && controller) {
        Dolly::Marker marker(frame, controller->PlayerCamera->GetFOVAngle(), pawn->Location,
                             RotatorToVector(pawn->Controller->Rotation));

        auto replaced = false;
        for (auto &m : markers) {
            if (m.Frame == marker.Frame) {
                m = marker;
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            markers.push_back(marker);
        }

        if (marker.Frame < 0) {
            ShiftTimeline(-marker.Frame);
        }

        FixTimeline();
    }
}

static void JumpFrames(int toJump) 
{
    if (frame + toJump > duration) {
        frame = duration;
        return;
    } else if (frame + toJump < 0) {
        frame = 0;
        return;
    }
    frame += toJump; 
}

static void DollyTab() {
    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();
    if (!pawn || !controller) {
        return;
    }

    Dolly dolly;

    if (playing) {
        if (ImGui::Button("Stop##dolly")) {
            playing = false;

            FixPlayer();
        }
    } else if (ImGui::Button("Play##dolly")) {
        if (frame >= duration) {
            frame = 0;
        }

        playing = true;
        FixPlayer();
        Menu::Hide();
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Camera View##dolly", &cameraView)) {
        FixPlayer();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Display Dolly Path##dolly", &pathDisplay);

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    if (recording) {
        if (ImGui::Button("Stop Recording##dolly")) {
            recording = false;
            recordings.push_back(currentRecording);
            currentRecording.Frames.clear();
            currentRecording.Frames.shrink_to_fit();

            FixTimeline();
        }
    } else if (ImGui::Button("Start Recording##dolly-record")) {
        currentRecording.StartFrame = frame;
        currentRecording.Character = character;
        Engine::SpawnCharacter(currentRecording.Character, currentRecording.Actor);
        recording = true;
    }

    static auto selectedCharacter = Engine::Characters[0];
    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    if (ImGui::BeginCombo("##dolly-character", selectedCharacter)) {
        for (auto i = 0; i < IM_ARRAYSIZE(Engine::Characters); ++i) {
            auto c = Engine::Characters[i];
            auto s = (c == selectedCharacter);
            if (ImGui::Selectable(c, s)) {
                selectedCharacter = c;
                character = static_cast<Engine::Character>(i);
            }

            if (s) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::InputInt("Frame##dolly", &frame);
    if (markers.size() == 0 && recordings.size() == 0) {
        frame = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Marker##dolly")) {
        AddMarker();
    }

    ImGui::SliderInt("Timeline##dolly", &frame, 0, duration);

    auto fov = controller->PlayerCamera->GetFOVAngle();
    if (ImGui::SliderFloat("FOV##dolly", &fov, 0, 160) && !cameraView) {
        controller->PlayerCamera->SetFOV(fov);
    }

    auto roll = static_cast<int>(controller->Rotation.Roll % 0x10000);
    if (ImGui::SliderInt("Roll##dolly", &roll, 0, 0x10000 - 1) && !cameraView) {
        forceRoll = true;
        controller->Rotation.Roll = roll;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Force Roll##dolly", &forceRoll);
    dolly.ForceRoll(cameraView || forceRoll);

    if (ImGui::CollapsingHeader("Markers##dolly")) {
        for (auto i = 0UL; i < markers.size(); ++i) {
            auto &marker = markers[i];

            ImGui::Text("Marker %d - %d", i, marker.Frame);

            char labelBuffer[0xFF];
            sprintf_s(labelBuffer, sizeof(labelBuffer), "##dolly-marker-%d", i);
            std::string label(labelBuffer);

            ImGui::SameLine();
            if (ImGui::Button(("Move" + label).c_str())) {
                marker.Frame = frame;

                if (marker.Frame < 0) {
                    ShiftTimeline(-marker.Frame);
                }

                FixTimeline();
            }

            ImGui::SameLine();
            if (ImGui::Button(("Goto" + label).c_str())) {
                frame = marker.Frame;
                cameraView = true;
            }

            ImGui::SameLine();
            if (ImGui::Button(("Delete" + label).c_str())) {
                markers.erase(markers.begin() + i);
                --i;

                FixTimeline();
            }
        }
        
        if (ImGui::Button("Save Markers")) {
            SaveMarkers();
        }

        
        if (ImGui::Button("Load Markers")) {
            LoadMarkers();
            FixTimeline();
        }
    }

    if (ImGui::CollapsingHeader("Recordings##dolly")) {
        for (auto i = 0UL; i < recordings.size(); ++i) {
            auto &rec = recordings[i];

            ImGui::Text("Recording %d - %s (%d - %d)", i,
                        Engine::Characters[static_cast<size_t>(rec.Character)], rec.StartFrame,
                        rec.StartFrame + rec.Frames.size() - 1);

            auto label = "##dolly-recording-" + std::to_string(i);

            ImGui::SameLine();
            if (ImGui::Button(("Move" + label).c_str())) {
                rec.StartFrame = frame;

                if (rec.StartFrame < 0) {
                    ShiftTimeline(-rec.StartFrame);
                }

                FixTimeline();
            }

            ImGui::SameLine();
            if (ImGui::Button(("Delete" + label).c_str())) {
                if (rec.Actor) {
                    Engine::Despawn(rec.Actor);
                    rec.Actor = nullptr;
                }

                recordings.erase(recordings.begin() + i);
                --i;

                FixTimeline();
            }
        }
        
        if (ImGui::Button("Save Recordings")) {
            SaveRecordings();
        }

       
        if (ImGui::Button("Load Recordings")) {
            LoadRecordings();
            FixTimeline();
        }
    }

     ImGui::SeparatorText("Keybinds##Keybinds-Separator");
    {
        ImGui::Checkbox("Toggle Reset Keybinds##Keybinds-ToggleResetKeybinds",
                        &ToggleResetKeybinds);
        ImGui::Separator(5.0f);

        // The last 2 parameters in ProcessHotkey needs to match the one in Initialize() function!
        ProcessHotkey("Add Marker##Dolly-maker", &MarkerKeybind, {"Dolly", "Tools", "MarkerKeybind"}, VK_F5);
        ProcessHotkey("Jump Frames (hold ctrl to jump back)##Dolly-jumpframes", &JumpFramesKeybind,{"Dolly", "Tools", "JumpFramesKeybind"}, VK_F6);
        ProcessHotkey("Start/stop##Dolly-startstop", &StartStopKeybind,{"Dolly", "Tools", "StartStopKeybind"}, VK_F7);
        ProcessHotkey("Start/stop Recording##Dolly-record", &RecordKeybind, {"Dolly", "Tools", "RecordKeybind"}, VK_F9);

        ProcessHotkey("FOV Increase##Dolly-fovplus", &FOVPlusKeybind, {"Dolly", "Tools", "FOVplus"},  VK_ADD);
        ProcessHotkey("FOV Decrease##Dolly-fovplus", &FOVMinusKeybind, {"Dolly", "Tools", "FOVminus"},  VK_SUBTRACT);
        ProcessHotkey("Roll Increase##Dolly-rollplus", &RollPlusKeybind,{"Dolly", "Tools", "Rollplus"}, VK_UP);
        ProcessHotkey("Roll Decrease##Dolly-rollminus", &RollMinusKeybind,{"Dolly", "Tools", "Rollminus"}, VK_DOWN);

        if (ImGui::InputInt("Frame Jump Amount##dolly", &FrameJumpAmount)) {
            Settings::SetSetting({"Dolly", "Tools", "FrameJumpAmount"}, FrameJumpAmount);
        }
    }
}

void SaveMarkers() {
    std::string path = GetAppDataPath() + "markers.dat";
    std::ofstream outFile(path, std::ios::binary);
    if (outFile.is_open()) {
        size_t markerCount = markers.size();
        outFile.write(reinterpret_cast<char *>(&markerCount), sizeof(markerCount));
        for (const auto &marker : markers) {
            outFile.write(reinterpret_cast<const char *>(&marker.Frame), sizeof(marker.Frame));
            outFile.write(reinterpret_cast<const char *>(&marker.FOV), sizeof(marker.FOV));
            outFile.write(reinterpret_cast<const char *>(&marker.Position),
                          sizeof(marker.Position));
            outFile.write(reinterpret_cast<const char *>(&marker.Rotation),
                          sizeof(marker.Rotation));
        }
        outFile.close();
    }
}

void LoadMarkers() {
    std::string path = GetAppDataPath() + "markers.dat";
    std::ifstream inFile(path, std::ios::binary);
    if (inFile.is_open()) {
        size_t markerCount;
        inFile.read(reinterpret_cast<char *>(&markerCount), sizeof(markerCount));

        markers.clear();
        for (size_t i = 0; i < markerCount; ++i) {
            int frame;
            float fov;
            Classes::FVector position;
            Classes::FVector rotation;

            inFile.read(reinterpret_cast<char *>(&frame), sizeof(frame));
            inFile.read(reinterpret_cast<char *>(&fov), sizeof(fov));
            inFile.read(reinterpret_cast<char *>(&position), sizeof(position));
            inFile.read(reinterpret_cast<char *>(&rotation), sizeof(rotation));

            Dolly::Marker marker(frame, fov, position, rotation);
            markers.push_back(marker);
        }
        inFile.close();
    }
}

void SaveRecordings() {
    std::string path = GetAppDataPath() + "recordings.dat";
    std::ofstream outFile(path, std::ios::binary);
    if (outFile.is_open()) {
        size_t recordingCount = recordings.size();
        outFile.write(reinterpret_cast<char *>(&recordingCount), sizeof(recordingCount));
        for (const auto &recording : recordings) {
            outFile.write(reinterpret_cast<const char *>(&recording.StartFrame),
                          sizeof(recording.StartFrame));
            outFile.write(reinterpret_cast<const char *>(&recording.Character),
                          sizeof(recording.Character));
            size_t frameCount = recording.Frames.size();
            outFile.write(reinterpret_cast<char *>(&frameCount), sizeof(frameCount));
            for (const auto &frame : recording.Frames) {
                outFile.write(reinterpret_cast<const char *>(&frame.Position),
                              sizeof(frame.Position));
                outFile.write(reinterpret_cast<const char *>(&frame.Rotation),
                              sizeof(frame.Rotation));
                outFile.write(reinterpret_cast<const char *>(frame.Bones), sizeof(frame.Bones));
            }
        }
        outFile.close();
    }
}

void LoadRecordings() {
    std::string path = GetAppDataPath() + "recordings.dat";
    std::ifstream inFile(path, std::ios::binary);
    if (inFile.is_open()) {
        size_t recordingCount;
        inFile.read(reinterpret_cast<char *>(&recordingCount), sizeof(recordingCount));

        recordings.clear();
        for (size_t i = 0; i < recordingCount; ++i) {
            Dolly::Recording recording;
            inFile.read(reinterpret_cast<char *>(&recording.StartFrame),
                        sizeof(recording.StartFrame));
            inFile.read(reinterpret_cast<char *>(&recording.Character),
                        sizeof(recording.Character));
            size_t frameCount;
            inFile.read(reinterpret_cast<char *>(&frameCount), sizeof(frameCount));
            recording.Frames.resize(frameCount);
            for (size_t j = 0; j < frameCount; ++j) {
                inFile.read(reinterpret_cast<char *>(&recording.Frames[j].Position),
                            sizeof(recording.Frames[j].Position));
                inFile.read(reinterpret_cast<char *>(&recording.Frames[j].Rotation),
                            sizeof(recording.Frames[j].Rotation));
                inFile.read(reinterpret_cast<char *>(recording.Frames[j].Bones),
                            sizeof(recording.Frames[j].Bones));
            }
            recordings.push_back(recording);
        }
        inFile.close();
    }
}

static void OnTick(float) {
    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (pawn && controller) {
        if (playing || cameraView) {
            if (markers.size() == 1) {
                auto &m = markers[0];
                pawn->Location = m.Position;
                pawn->Controller->Rotation = VectorToRotator(m.Rotation);
            } else if (markers.size() > 1) {
                for (auto i = static_cast<int>(markers.size() - 1); i >= 0; --i) {
                    auto &m0 = markers[i];
                    if (m0.Frame <= frame) {
                        if (i == markers.size() - 1) {
                            controller->PlayerCamera->SetFOV(m0.FOV);
                            pawn->Location = m0.Position;
                            pawn->Controller->Rotation = VectorToRotator(m0.Rotation);
                        } else {
                            auto &m1 = markers[i + 1];

                            Classes::FVector pos;
                            Classes::FVector rot;

                            for (auto p = 0; p < 3; ++p) {
                                auto fieldOffset =
                                    FIELD_OFFSET(Dolly::Marker, Position.X) + (p * sizeof(float));
                                auto s0 = Slope(i, fieldOffset);
                                auto s1 = Slope(i + 1, fieldOffset);

                                (&pos.X)[p] = Interpolate(static_cast<float>(m0.Frame),
                                                          static_cast<float>(m1.Frame),
                                                          GetMarkerField(i, fieldOffset),
                                                          GetMarkerField(i + 1, fieldOffset), s0,
                                                          s1, static_cast<float>(frame));
                            }

                            for (auto r = 0; r < 3; ++r) {
                                auto fieldOffset =
                                    FIELD_OFFSET(Dolly::Marker, Rotation.X) + (r * sizeof(float));
                                auto s0 = Slope(i, fieldOffset);
                                auto s1 = Slope(i + 1, fieldOffset);

                                (&rot.X)[r] = Interpolate(static_cast<float>(m0.Frame),
                                                          static_cast<float>(m1.Frame),
                                                          GetMarkerField(i, fieldOffset),
                                                          GetMarkerField(i + 1, fieldOffset), s0,
                                                          s1, static_cast<float>(frame));
                            }

                            controller->PlayerCamera->SetFOV(Interpolate(
                                static_cast<float>(m0.Frame), static_cast<float>(m1.Frame),
                                GetMarkerField(i, FIELD_OFFSET(Dolly::Marker, FOV)),
                                GetMarkerField(i + 1, FIELD_OFFSET(Dolly::Marker, FOV)),
                                Slope(i, FIELD_OFFSET(Dolly::Marker, FOV)),
                                Slope(i + 1, FIELD_OFFSET(Dolly::Marker, FOV)),
                                static_cast<float>(frame)));

                            pawn->Location = pos;
                            controller->Rotation = VectorToRotator(rot);
                        }

                        break;
                    }
                }
            }

            pawn->Velocity = {0};
            pawn->Acceleration = {0};
            pawn->Health = pawn->MaxHealth;

            for (auto i = 0UL; i < pawn->Timers.Num(); ++i) {
                pawn->Timers[i].Count = 0;
            }

            if (playing && ++frame > duration) {
                frame = 0;
                playing = false;

                FixPlayer();
            }
        }

        if (recording) {
            Dolly::Recording::Frame f;
            f.Position = pawn->Location;
            f.Rotation = pawn->Rotation;
            memcpy(f.Bones, pawn->Mesh3p->LocalAtoms.Buffer(), sizeof(f.Bones));

            currentRecording.Frames.push_back(f);
        }
    }
}

static void OnRender(IDirect3DDevice9 *device) {
    if (!playing && pathDisplay) {
        auto window = ImGui::BeginRawScene("##dolly-backbuffer");

        if (markers.size() > 1) {
            for (auto i = 0UL; i < markers.size() - 1; ++i) {
                auto &m0 = markers[i];
                auto &m1 = markers[i + 1];

                float s0[3];
                float s1[3];
                for (auto p = 0; p < 3; ++p) {
                    auto fieldOffset =
                        FIELD_OFFSET(Dolly::Marker, Position.X) + (p * sizeof(float));
                    s0[p] = Slope(i, fieldOffset);
                    s1[p] = Slope(i + 1, fieldOffset);
                }

                for (float t = static_cast<float>(m0.Frame); t < m1.Frame; t += 5) {
                    Classes::FVector pos;
                    for (auto p = 0; p < 3; ++p) {
                        auto fieldOffset =
                            FIELD_OFFSET(Dolly::Marker, Position.X) + (p * sizeof(float));
                        (&pos.X)[p] =
                            Interpolate(static_cast<float>(m0.Frame), static_cast<float>(m1.Frame),
                                        GetMarkerField(i, fieldOffset),
                                        GetMarkerField(i + 1, fieldOffset), s0[p], s1[p], t);
                    }

                    if (Engine::WorldToScreen(device, pos)) {
                        window->DrawList->AddCircleFilled(ImVec2(pos.X, pos.Y), 2000.0f / pos.Z,
                                                          ImColor(ImVec4(1, 0, 0, 1)));
                    }
                }
            }
        }

        for (auto &m : markers) {
            auto pos = m.Position;
            if (Engine::WorldToScreen(device, pos)) {
                auto markerSize = 7500.0f / pos.Z;

                ImVec2 topLeft(pos.X - markerSize, pos.Y - markerSize);
                ImVec2 bottomRight(pos.X + markerSize, pos.Y + markerSize);
                window->DrawList->AddRectFilled(topLeft, bottomRight,
                                                ImColor(ImVec4(0.18f, 0.31f, 0.31f, 1)));
            }
        }

        ImGui::EndRawScene();
    }
}

static void OnInput(unsigned int &msg, int keycode) {
    if (msg == WM_KEYDOWN) {
        if (keycode == MarkerKeybind) {
            AddMarker();
        }

        if (keycode == JumpFramesKeybind && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
            JumpFrames(FrameJumpAmount);
        }
        if (keycode == JumpFramesKeybind && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
            JumpFrames(-FrameJumpAmount);
        }
        
        if (keycode == StartStopKeybind) {
            if (playing) {
                playing = false;
                FixPlayer();
            } else if(!playing){
                if (frame >= duration) {
                    frame = 0;
                }

                playing = true;
                FixPlayer();
            }
        }

        // THIS IS VERY INEFFICIENT, CUZ IT'S SETTING THE CONTROLLER ON EVERY SINGLE KEYPRESS!! (but
        // i'm lazy as fuck so wontfix)
        if (keycode == FOVPlusKeybind) {
            auto controller = Engine::GetPlayerController();
            auto fov = controller->PlayerCamera->GetFOVAngle();
            if ((fov + 1) >= 160) {
                return;
            }
            fov += 1;
            controller->PlayerCamera->SetFOV(fov);
        }
        if (keycode == FOVMinusKeybind) {
            auto controller = Engine::GetPlayerController();
            auto fov = controller->PlayerCamera->GetFOVAngle();
            if (fov - 1 <= 0) return;
            fov -= 1;
            controller->PlayerCamera->SetFOV(fov);
        }
        if (keycode == RollPlusKeybind) {
            forceRoll = true;
            auto controller = Engine::GetPlayerController();
            auto roll = controller->Rotation.Roll;
            roll += 50;
            controller->Rotation.Roll = roll;
        }
        if (keycode == RollMinusKeybind) {
            forceRoll = true;
            auto controller = Engine::GetPlayerController();
            auto roll = controller->Rotation.Roll;
            roll -= 50;
            controller->Rotation.Roll = roll;
        }

        if (keycode == RecordKeybind) {
            if (recording) {
                recording = false;
                recordings.push_back(currentRecording);
                currentRecording.Frames.clear();
                currentRecording.Frames.shrink_to_fit();
                FixTimeline();
            } else {
                currentRecording.StartFrame = frame;
                currentRecording.Character = character;
                Engine::SpawnCharacter(currentRecording.Character, currentRecording.Actor);
                recording = true;
            }
        }
    }
}

Dolly::~Dolly() { // IF I REMOVE THIS FUNCTION, THE GAME CRASHES ON EXIT
   // Ensure proper cleanup of resources
   //markers.clear();
   //recordings.clear();
   //currentRecording.Frames.clear();
   if (forceRollPatch) {
       //VirtualProtect(forceRollPatch, sizeof(forceRollPatchOriginal), PAGE_EXECUTE_READWRITE, &oldProtect);
       //memcpy(forceRollPatch, forceRollPatchOriginal, sizeof(forceRollPatchOriginal));
   }
}

bool Dolly::Initialize() {

    MarkerKeybind = Settings::GetSetting({"Dolly", "Tools", "MarkerKeybind"}, VK_F5);
    JumpFramesKeybind = Settings::GetSetting({"Dolly", "Tools", "JumpFramesKeybind"}, VK_F6);
    RecordKeybind = Settings::GetSetting({"Dolly", "Tools", "RecordKeybind"}, VK_F9);
    StartStopKeybind = Settings::GetSetting({"Dolly", "Tools", "StartStopKeybind"}, VK_F9);
    FrameJumpAmount = Settings::GetSetting({"Dolly", "Tools", "FrameJumpAmount"}, 100);
    FOVPlusKeybind = Settings::GetSetting({"Dolly", "Tools", "FOVplus"}, VK_ADD);
    FOVMinusKeybind = Settings::GetSetting({"Dolly", "Tools", "FOVminus"}, VK_SUBTRACT);
    RollPlusKeybind = Settings::GetSetting({"Dolly", "Tools", "Rollplus"}, VK_UP);
    RollMinusKeybind = Settings::GetSetting({"Dolly", "Tools", "Rollminus"}, VK_DOWN);

    forceRollPatch = Pattern::FindPattern("\x89\x93\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x83\xB8",
                                          "xx????x????xx");
    if (!forceRollPatch) {
        MessageBoxA(nullptr, "Failed to find forceRollPatch", "Failure", 0);
        return false;
    }

    unsigned long oldProtect;
    if (!VirtualProtect(forceRollPatch, sizeof(forceRollPatchOriginal), PAGE_EXECUTE_READWRITE,
                        &oldProtect)) {
        MessageBoxA(nullptr, "Failed to change page protection for rollPatch", "Failure", 0);
        return false;
    }

    memcpy(forceRollPatchOriginal, forceRollPatch, sizeof(forceRollPatchOriginal));

    Menu::AddTab("Dolly", DollyTab);
    Engine::OnTick(OnTick);
    Engine::OnRenderScene(OnRender);
    Engine::OnInput(OnInput);

    Engine::OnActorTick([](Classes::AActor *actor) {
        if (!actor) {
            return;
        }

        if (hideQueued) {
            hideQueued = false;

            auto pawn = Engine::GetPlayerPawn();
            if (pawn) {
                auto hide = playing || cameraView;
                pawn->Mesh1p->SetHidden(hide);
                pawn->Mesh1pLowerBody->SetHidden(hide);
                pawn->Mesh3p->SetHidden(hide);

                pawn->SetCollisionType(hide ? Classes::ECollisionType::COLLIDE_NoCollision
                                            : Classes::ECollisionType::COLLIDE_BlockAllButWeapons);
            }
        }

        for (auto &r : recordings) {
            if (r.Actor == actor) {
                if (frame >= r.StartFrame &&
                    frame < r.StartFrame + static_cast<int>(r.Frames.size())) {
                    auto &f = r.Frames[frame - r.StartFrame];
                    r.Actor->Location = f.Position;
                    r.Actor->Rotation = f.Rotation;
                } else {
                    r.Actor->Location = {0};
                }
            }
        }
    });

    Engine::OnBonesTick([](Classes::TArray<Classes::FBoneAtom> *bones) {
        for (auto &r : recordings) {
            if (r.Actor && r.Actor->SkeletalMeshComponent &&
                r.Actor->SkeletalMeshComponent->LocalAtoms.Buffer() == bones->Buffer() &&
                frame >= r.StartFrame && frame < r.StartFrame + static_cast<int>(r.Frames.size())) {
                Engine::TransformBones(r.Character, bones, r.Frames[frame - r.StartFrame].Bones);
            }
        }
    });

    Engine::OnPreLevelLoad([](const wchar_t *levelName) {
        for (auto &r : recordings) {
            r.Actor = nullptr;
        }
    });

    Engine::OnPostLevelLoad([](const wchar_t *levelName) {
        for (auto &r : recordings) {
            Engine::SpawnCharacter(r.Character, r.Actor);
        }
    });

    return true;
}

std::string Dolly::GetName() { return "Dolly"; }