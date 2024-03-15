#include <vector>
#include <locale>
#include <codecvt>
#include <d3d9.h>

#include "debug.h"
#include "engine.h"
#include "settings.h"
#include "imgui/imgui.h"
#include "imgui/imgui_mmultiplayer.h"
#include "menu.h"
#include "mmultiplayer.h"

#if MMULTIPLAYER_DEBUG_IMGUI
static bool imGuiWindowOpen = false;
#endif

#if MMULTIPLAYER_DEBUG_SPINNERS
static bool imSpinnersWindowOpen = false;

#define IMSPINNER_DEMO
#include "imgui/imgui_spinner.h"
#endif

static auto show = false;
static std::vector<MenuTab> tabs;
static int showKeybind = 0;
static std::wstring levelName;

static void RenderMenu(IDirect3DDevice9 *device) {
	if (show) {
		ImGui::Begin("MMultiplayer " MMULTIPLAYER_VERSION);
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

#if MMULTIPLAYER_DEBUG_IMGUI
	if (imGuiWindowOpen)
	{
		ImGui::ShowDemoWindow(&imGuiWindowOpen);
	}
#endif
#if MMULTIPLAYER_DEBUG_SPINNERS
	if (imSpinnersWindowOpen && ImGui::Begin("ImSpinners Demo", &imSpinnersWindowOpen))
	{
		ImSpinner::demoSpinners();
		ImGui::End();
	}
#endif
}

#if MMULTIPLAYER_DEBUG_INFO
static void DebugTab() {
	ImGui::SeparatorText("Versions");
	ImGui::Text("ImGui: %s", IMGUI_VERSION);

	// I don't know what version this is in. Nothing can be found in the file or on github
	ImGui::Text("ImSpinner: Unknown");
	ImGui::Text("Nlohmann Json: %d.%d.%d", NLOHMANN_JSON_VERSION_MAJOR, NLOHMANN_JSON_VERSION_MINOR, NLOHMANN_JSON_VERSION_PATCH);
	ImGui::SpacingY();

#if MMULTIPLAYER_DEBUG_IMGUI
	if (ImGui::Button("ImGui##debug-imgui"))
	{
		imGuiWindowOpen = !imGuiWindowOpen;
	}
#endif

#if MMULTIPLAYER_DEBUG_SPINNERS
	if (ImGui::Button("ImSpinners##debug-imspinners"))
	{
		imSpinnersWindowOpen = !imSpinnersWindowOpen;
	}
#endif
}
#endif

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

	ImGui::SpacingY();

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

	ImGui::Text("Time Dilation");
	ImGui::InputFloat("##world-time-dilation", &world->TimeDilation);

	ImGui::Text("Gravity");
	ImGui::InputFloat("##world-gravity", &world->WorldGravityZ);

	if (levelName.empty()) {
		levelName = world->GetMapName(false).c_str();
	}

	ImGui::SpacingY();
	auto levels = world->StreamingLevels;
	if (ImGui::TreeNode("world##world-levels", "%ws (%d)", levelName.c_str(), levels.Num())) 
	{
		for (auto i = 0UL; i < levels.Num(); ++i) {
			auto level = levels.GetByIndex(i);
			if (level) {
				bool check = level->bShouldBeLoaded;
				auto label = level->PackageName.GetName();

				if (level->PackageName.Number > 0)
				{
					label += "_" + std::to_string(level->PackageName.Number - 1);
				}

				ImGui::Checkbox(label.c_str(), &check);

				if (check) {
					level->bShouldBeLoaded = level->bShouldBeVisible = true;
				} else {
					level->bShouldBeLoaded = false;
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

#if MMULTIPLAYER_DEBUG_INFO
	AddTab("Debug", DebugTab);
#endif
	AddTab("Engine", EngineTab);
	AddTab("World", WorldTab);

	return true;
}