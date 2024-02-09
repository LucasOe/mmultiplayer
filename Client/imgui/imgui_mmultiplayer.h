#pragma once

#include "imgui_internal.h"

namespace ImGui {
	ImGuiWindow* BeginRawScene(const char *name);
	void EndRawScene();
	bool Hotkey(const char *label, int *k, const ImVec2 &size_arg = ImVec2(0, 0));
	void SeperatorWithPadding(float paddingVeritcal);
} 
