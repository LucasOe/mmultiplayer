#include "tag.h"
#include "client.h"

#include <codecvt>
#include <algorithm>

#include "../engine.h"
#include "../menu.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../util.h"

// clang-format off

static auto showOverlay = true;
static std::string levelName;

static void TagTab() { 
	ImGui::Checkbox("Show Overlay##tag-overlay", &showOverlay);
}

static void OnRender(IDirect3DDevice9 *device) {
	if (!showOverlay) {
		return;
	}

	auto pawn = Engine::GetPlayerPawn();
	auto controller = Engine::GetPlayerController();
	auto world = Engine::GetWorld();

	if (!pawn || !controller || !world) {
		return;
	}

	if (levelName.empty() || levelName == "tdmainmenu") {
		return;
	}

	int playersInTheSameLevel = 0;
	auto players = Client::GetPlayerList();

	for (size_t i = 0; i < players.size(); i++) {
		if (players[i]->Level == levelName && players[i]->Actor) {
			playersInTheSameLevel++;
		}
	}

	if (playersInTheSameLevel == 0) {
		return;
	}
	
	auto window = ImGui::BeginRawScene("##tag-info");
	auto &io = ImGui::GetIO();

	static float padding = 5.0f;
	static float rightPadding = 90.0f;

	float yIncrement = ImGui::GetTextLineHeight();
	float y = (playersInTheSameLevel * yIncrement) - yIncrement + (padding / 2);
	auto color = ImColor(ImVec4(1, 1, 1, 1));

	window->DrawList->AddRectFilled(ImVec2(), ImVec2(256, y + padding + yIncrement), ImColor(ImVec4(0, 0, 0, 0.4f)));

	char buffer[0x200] = {0};

	for (size_t i = 0; i < players.size(); i++) {
		if (!players[i]->Actor || players[i]->Level != levelName) {
			continue;
		}

		float dist = Distance(players[i]->Actor->Location, pawn->Location);

		if (dist >= 10.0f) {
			sprintf_s(buffer, "%.0f m", dist);
		} else {
			sprintf_s(buffer, "%.1f m", dist);
		}

		window->DrawList->AddText(ImVec2(padding, y), color, buffer);
		window->DrawList->AddText(ImVec2(rightPadding, y), color, players[i]->Name.c_str());	 	

		y -= yIncrement;
	}

	ImGui::EndRawScene();
}

bool Tag::Initialize() 
{
	Menu::AddTab("Tag", TagTab);
	Engine::OnRenderScene(OnRender);

	Engine::OnPreLevelLoad([](const wchar_t *levelNameW) {
		levelName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(levelNameW);

		std::transform(levelName.begin(), levelName.end(), levelName.begin(), [](char c) { return tolower(c); });
	});

	return true;
}

std::string Tag::GetName() { return "Tag"; }