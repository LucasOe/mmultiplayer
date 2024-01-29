#include <vector>
#include <locale>
#include <codecvt>
#include <d3d9.h>

#include "debug.h"
#include "engine.h"
#include "settings.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "menu.h"

static auto show = false;
static std::vector<MenuTab> tabs;
static int showKeybind = 0;
static std::wstring levelName;

static void RenderMenu(IDirect3DDevice9 *device) {
	if (show) {
		ImGui::Begin("MMultiplayer 2.1.3");
		ImGui::BeginTabBar("");

		for (auto tab : tabs) {
			if (ImGui::BeginTabItem(tab.Name.c_str())) {
				tab.Callback();
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
		ImGui::End();
	}
}

/*** Basic Tabs ***/
static void EngineTab() {
	auto engine = Engine::GetEngine();
	if (!engine) {
		return;
	}

	static char command[0xFFF] = { 0 };

	auto commandInputCallback = []() {
		if (command[0]) {
			Engine::ExecuteCommand(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(command).c_str());

			command[0] = 0;
		}
	};

	if (ImGui::InputText("##command", command, sizeof(command), ImGuiInputTextFlags_EnterReturnsTrue)) {
		commandInputCallback();
	}

	ImGui::SameLine();
	if (ImGui::Button("Execute Command##engine-execute-command")) {
		commandInputCallback();
	}

	ImGui::SeperatorWithPadding(2.5f);

	bool check = engine->bSmoothFrameRate;
	ImGui::Checkbox("Smooth Framerate##engine-smooth-framerate", &check);
	engine->bSmoothFrameRate = check;
	if (check) {
		ImGui::InputFloat("Min Smoothed Framerate##engine-max-smoothed", &engine->MinSmoothedFrameRate);
		ImGui::InputFloat("Max Smoothed Framerate##engine-min-smoothed", &engine->MaxSmoothedFrameRate);
	}

	auto client = engine->Client;
	if (client) {
		ImGui::InputFloat("Gamma##engine-gamma", &client->DisplayGamma);
	}

	ImGui::SeperatorWithPadding(2.5f);

	if (ImGui::Hotkey("Menu Keybind##menu-show", &showKeybind)) {
		Settings::SetSetting("menu", "showKeybind", showKeybind);
	}

	ImGui::SameLine();

	if (ImGui::Button("Reset##showKeybind")) {
		Settings::SetSetting("menu", "showKeybind", showKeybind = VK_INSERT);
	}

	ImGui::SameLine();

	if (ImGui::Button("Debug Console##client-show-console")) {
		Debug::CreateConsole();
	}
	
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
		ImGui::SetTooltip("Creates a console window that will display debug info.\nIf you're trying to close the debug console, it will close Mirror's Edge too.");
	}
}

static void WorldTab() {
	auto world = Engine::GetWorld();
	if (!world) {
		return;
	}

	ImGui::InputFloat("Time Dilation##world-time-dilation", &world->TimeDilation);
	ImGui::InputFloat("Gravity##-world-gravity", &world->WorldGravityZ);

	if (levelName.empty()) {
		levelName = world->GetMapName(false).c_str();
	}

	ImGui::SeperatorWithPadding(2.5f);

	auto levels = world->StreamingLevels;
	if (ImGui::TreeNode("world##world-levels", "%ws (%d)", levelName.c_str(), levels.Num())) {
		for (auto i = 0UL; i < levels.Num(); ++i) {
			auto level = levels.GetByIndex(i);
			if (level) {
				bool check = level->bShouldBeLoaded;
				auto label = level->PackageName.GetName();

				if (level->PackageName.Number > 0) {
					label += "_" + std::to_string(level->PackageName.Number - 1);
				}

				if (ImGui::Checkbox(label.c_str(), &check)) {
					level->bShouldBeLoaded = level->bShouldBeVisible = check;
				}
			}
		}

		ImGui::TreePop();
	}
}

void Menu::AddTab(const char *name, MenuTabCallback callback) {
	tabs.push_back({ name, callback });
}

void Menu::Hide() {
	show = false;
	Engine::BlockInput(false);
}

void Menu::Show() {
	show = true;
	Engine::BlockInput(true);
}

bool Menu::Initialize() {
	showKeybind = Settings::GetSetting("menu", "showKeybind", VK_INSERT);

	Engine::OnRenderScene(RenderMenu);

	Engine::OnInput([](unsigned int &msg, int keycode) {
		if (!show && msg == WM_KEYUP && keycode == showKeybind) {
			Show();
		}
	});

	Engine::OnSuperInput([](unsigned int &msg, int keycode) {
		if (show && msg == WM_KEYUP && (keycode == showKeybind || keycode == VK_ESCAPE)) {
			Hide();
		}
	});

	Engine::OnPostLevelLoad([](const wchar_t *newLevelName) {
		levelName = newLevelName;
	});

	AddTab("Engine", EngineTab);
	AddTab("World", WorldTab);

	return true;
}