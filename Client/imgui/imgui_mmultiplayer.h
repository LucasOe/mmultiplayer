#pragma once

#include "imgui_internal.h"

namespace ImGui {
	ImGuiWindow* BeginRawScene(const char *name);
	void EndRawScene();
	bool Hotkey(const char *label, int *k, const ImVec2 &size_arg = ImVec2(0, 0));

	void SpacingY(float y = 2.5f);
	void HotkeyResettable(const char* text, const char* label, int* keyBind, int defaultKeyBind, const char* sub, const char* key, bool showToggleResetKey);
	void HelpMarker(const char* desc);
} 
