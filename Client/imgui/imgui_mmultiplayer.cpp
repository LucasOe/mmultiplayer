#pragma once

#include "imgui_mmultiplayer.h"
#include <windows.h>

namespace ImGui {
    ImGuiWindow *BeginRawScene(const char *name)  { 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        auto &io = ImGui::GetIO();
        ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::GetCurrentWindow()->DrawList->PushClipRectFullScreen();

        return ImGui::GetCurrentWindow();
    }

    void EndRawScene() { 
        ImGui::PopClipRect();
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    static const char *VK_TO_KEY[] = {
        "None",              // 0
        "LeftMouseButton",   // 1
        "RightMouseButton",  // 2
        "Cancel",            // 3
        "MiddleMouseButton", // 4
        "ThumbMouseButton",  // 5
        "ThumbMouseButton2", // 6
        "",                  // 7
        "BackSpace",         // 8
        "TAB",               // 9
        "",                  // a
        "",                  // b
        "Clear",             // c
        "Enter",             // d
        "",                  // e
        "",                  // f
        "Shift",             // 10
        "Control",           // 11
        "Alt",               // 12
        "Pause",             // 13
        "CAPSLOCK",          // 14
        "",                  // 15
        "",                  // 16
        "",                  // 17
        "",                  // 18
        "",                  // 19
        "",                  // 1a
        "Escape",            // 1b
        "",                  // 1c
        "",                  // 1d
        "",                  // 1e
        "",                  // 1f
        "SpaceBar",          // 20
        "PageUp",            // 21
        "PageDown",          // 22
        "End",               // 23
        "Home",              // 24
        "Left",              // 25
        "Up",                // 26
        "Right",             // 27
        "Down",              // 28
        "Select",            // 29
        "Print",             // 2a
        "Execute",           // 2b
        "",                  // 2c
        "Insert",            // 2d
        "Delete",            // 2e
        "Help",              // 2f
        "0",                 // 30
        "1",                 // 31
        "2",                 // 32
        "3",                 // 33
        "4",                 // 34
        "5",                 // 35
        "6",                 // 36
        "7",                 // 37
        "8",                 // 38
        "9",                 // 39
        "",                  // 3a
        "",                  // 3b
        "",                  // 3c
        "",                  // 3d
        "",                  // 3e
        "",                  // 3f
        "",                  // 40
        "A",                 // 41
        "B",                 // 42
        "C",                 // 43
        "D",                 // 44
        "E",                 // 45
        "F",                 // 46
        "G",                 // 47
        "H",                 // 48
        "I",                 // 49
        "J",                 // 4a
        "K",                 // 4b
        "",                  // 4c
        "M",                 // 4d
        "N",                 // 4e
        "O",                 // 4f
        "P",                 // 50
        "Q",                 // 51
        "R",                 // 52
        "S",                 // 53
        "T",                 // 54
        "U",                 // 55
        "V",                 // 56
        "W",                 // 57
        "X",                 // 58
        "Y",                 // 59
        "Z",                 // 5a
        "",                  // 5b
        "",                  // 5c
        "",                  // 5d
        "",                  // 5e
        "",                  // 5f
        "NumPadZero",        // 60
        "NumPadOne",         // 61
        "NumPadTwo",         // 62
        "NumPadThree",       // 63
        "NumPadFour",        // 64
        "NumPadFive",        // 65
        "NumPadSix",         // 66
        "NumPadSeven",       // 67
        "NumPadEight",       // 68
        "NumPadNine",        // 69
        "Multiply",          // 6a
        "Add",               // 6b
        "",                  // 6c
        "Subtract",          // 6d
        "Decima",            // 6e
        "Divide",            // 6f
        "F1",                // 70
        "F2",                // 71
        "F3",                // 72
        "F4",                // 73
        "F5",                // 74
        "F6",                // 75
        "F7",                // 76
        "F8",                // 77
        "F9",                // 78
        "F10",               // 79
        "F11",               // 7a
        "F12",               // 7b
        "F13",               // 7c
        "F14",               // 7d
        "F15",               // 7e
        "F16",               // 7f
        "F17",               // 80
        "F18",               // 81
        "F19",               // 82
        "F20",               // 83
        "F21",               // 84
        "F22",               // 85
        "F23",               // 86
        "F24",               // 87
        "",                  // 88
        "",                  // 89
        "",                  // 8a
        "",                  // 8b
        "",                  // 8c
        "",                  // 8d
        "",                  // 8e
        "",                  // 8f
        "NumLock",           // 90
        "ScrollLock",        // 91
        "",                  // 92
        "",                  // 93
        "",                  // 94
        "",                  // 95
        "",                  // 96
        "",                  // 97
        "",                  // 98
        "",                  // 99
        "",                  // 9a
        "",                  // 9b
        "",                  // 9c
        "",                  // 9d
        "",                  // 9e
        "",                  // 9f
        "LeftShift",         // a0
        "RightShift",        // a1
        "LeftContro",        // a2
        "RightContro",       // a3
        "LeftAlt",           // a4
        "RightAlt",          // a5
        "",                  // a6
        "",                  // a7
        "",                  // a8
        "",                  // a9
        "",                  // aa
        "",                  // ab
        "",                  // ac
        "",                  // ad
        "",                  // ae
        "",                  // af
        "",                  // b0
        "",                  // b1
        "",                  // b2
        "",                  // b3
        "",                  // b4
        "",                  // b5
        "",                  // b6
        "",                  // b7
        "",                  // b8
        "",                  // b9
        "",                  // ba
        "",                  // bb
        "",                  // bc
        "",                  // bd
        "",                  // be
        "",                  // bf
        "",                  // c0
        "",                  // c1
        "",                  // c2
        "",                  // c3
        "",                  // c4
        "",                  // c5
        "",                  // c6
        "",                  // c7
        "",                  // c8
        "",                  // c9
        "",                  // ca
        "",                  // cb
        "",                  // cc
        "",                  // cd
        "",                  // ce
        "",                  // cf
        "",                  // d0
        "",                  // d1
        "",                  // d2
        "",                  // d3
        "",                  // d4
        "",                  // d5
        "",                  // d6
        "",                  // d7
        "",                  // d8
        "",                  // d9
        "",                  // da
        "LeftBracket",       // db
        "Backslash",         // dc
        "RightBracket",      // dd
        "Quote",             // de
        "",                  // df
        "",                  // e0
        "",                  // e1
        "",                  // e2
        "",                  // e3
        "",                  // e4
        "",                  // e5
        "",                  // e6
        "",                  // e7
        "",                  // e8
        "",                  // e9
        "",                  // ea
        "",                  // eb
        "",                  // ec
        "",                  // ed
        "",                  // ee
        "",                  // ef
        "",                  // f0
        "",                  // f1
        "",                  // f2
        "",                  // f3
        "",                  // f4
        "",                  // f5
        "",                  // f6
        "",                  // f7
        "",                  // f8
        "",                  // f9
        "",                  // fa
        "",                  // fb
        "",                  // fc
        "",                  // fd
        "",                  // fe
    };

    bool Hotkey(const char *label, int *k, const ImVec2 &size_arg) {
        if (*k < 0 || *k >= sizeof(VK_TO_KEY) / sizeof(VK_TO_KEY[0])) {
            *k = 0;
        }

        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g = *GImGui;
        ImGuiIO &io = g.IO;
        const ImGuiStyle &style = g.Style;

        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), label_size.y + style.FramePadding.y * 2.0f);

        const ImRect frame_bb(window->DC.CursorPos + ImVec2(label_size.x + style.ItemInnerSpacing.x, 0.0f), window->DC.CursorPos + size);
        const ImRect total_bb(window->DC.CursorPos, frame_bb.Max);

        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id))
            return false;

        const bool focus_requested = g.ActiveId == id;
        const bool hovered = ImGui::ItemHoverable(frame_bb, id, ImGuiActivateFlags_None);

        if (hovered) {
            ImGui::SetHoveredID(id);
            g.MouseCursor = ImGuiMouseCursor_TextInput;
        }

        const bool user_clicked = hovered && io.MouseClicked[0];

        if (focus_requested || user_clicked) {
            if (g.ActiveId != id) {
                // Start edition
                memset(io.MouseDown, 0, sizeof(io.MouseDown));
                memset(io.KeysDown, 0, sizeof(io.KeysDown));
                *k = 0;
            }
            ImGui::SetActiveID(id, window);
            ImGui::FocusWindow(window);
        } else if (io.MouseClicked[0]) {
            // Release focus when we click outside
            if (g.ActiveId == id)
                ImGui::ClearActiveID();
        }

        bool value_changed = false;
        int key = *k;

        if (g.ActiveId == id) {
            for (auto i = 0; i < 5; i++) {
                if (io.MouseDown[i]) {
                    switch (i) {
                    case 0:
                        key = VK_LBUTTON;
                        break;
                    case 1:
                        key = VK_RBUTTON;
                        break;
                    case 2:
                        key = VK_MBUTTON;
                        break;
                    case 3:
                        key = VK_XBUTTON1;
                        break;
                    case 4:
                        key = VK_XBUTTON2;
                        break;
                    }
                    value_changed = true;
                    ImGui::ClearActiveID();
                }
            }
            if (!value_changed) {
                for (auto i = VK_BACK; i <= VK_RMENU; i++) {
                    if (io.KeysDown[i]) {
                        key = i;
                        value_changed = true;
                        ImGui::ClearActiveID();
                    }
                }
            }

            if (IsKeyPressedMap(ImGuiKey_Escape)) {
                *k = 0;
                ImGui::ClearActiveID();
            } else {
                *k = key;
            }
        }

        char buf_display[64] = "None";
        ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImVec4(0.20f, 0.25f, 0.30f, 1.0f)), true, style.FrameRounding);

        if (*k != 0 && g.ActiveId != id) {
            strcpy_s(buf_display, VK_TO_KEY[*k]);
        } else if (g.ActiveId == id) {
            strcpy_s(buf_display, "<Press a key>");
        }

        const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x,frame_bb.Min.y + size.y);
        ImVec2 render_pos = frame_bb.Min + style.FramePadding;
        ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding, buf_display, NULL, NULL, style.ButtonTextAlign, &clip_rect);

        if (label_size.x > 0)
            ImGui::RenderText(ImVec2(total_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), label);

        return value_changed;
    }

    void SeperatorWithPadding(float paddingVeritcal) { 
        ImGui::Dummy(ImVec2(0.0f, paddingVeritcal));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, paddingVeritcal));
    }
}