#include <vector>
#include <locale>
#include <codecvt>
#include <d3d9.h>

#include "debug.h"
#include "engine.h"
#include "settings.h"
#include "imgui/imgui.h"
#include "menu.h"

static bool ShowMenu = false;
static int ShowKeybind = 0;
static std::vector<MenuTab> Tabs;
static std::wstring LevelName;

static struct {
	std::vector<MenuCallback> Callbacks;
} menu;

static void RenderMenu(IDirect3DDevice9 *device) 
{
	if (!ShowMenu)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(720, 420), ImGuiCond_FirstUseEver);
	ImGui::Begin("MMultiplayer 2.3.0-alpha.4", NULL, ImGuiWindowFlags_NoCollapse);
	ImGui::BeginTabBar("");

	for (auto tab : Tabs) 
	{
		if (ImGui::BeginTabItem(tab.Name.c_str())) 
		{
			tab.Callback();
			ImGui::EndTabItem();
		}
	}

	ImGui::EndTabBar();
	ImGui::End();
}

static void EngineTab() 
{
	auto engine = Engine::GetEngine();
	if (!engine) 
	{
		return;
	}

	static char command[0xFFF] = { 0 };

	auto commandInputCallback = []() 
	{
		if (command[0]) 
		{
			Engine::ExecuteCommand(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(command).c_str());

			command[0] = 0;
		}
	};

	if (ImGui::InputText("##command", command, sizeof(command), ImGuiInputTextFlags_EnterReturnsTrue)) 
	{
		commandInputCallback();
	}

	ImGui::SameLine();
	if (ImGui::Button("Execute Command##engine-execute-command")) 
	{
		commandInputCallback();
	}

	ImGui::Separator(5.0f);

	bool check = engine->bSmoothFrameRate;
	ImGui::Checkbox("Smooth Framerate##engine-smooth-framerate", &check);
	engine->bSmoothFrameRate = check;
	if (check) 
	{
		ImGui::InputFloat("Min Smoothed Framerate##engine-max-smoothed", &engine->MinSmoothedFrameRate);
		ImGui::InputFloat("Max Smoothed Framerate##engine-min-smoothed", &engine->MaxSmoothedFrameRate);
	}

	auto client = engine->Client;
	if (client) 
	{
		ImGui::InputFloat("Gamma##engine-gamma", &client->DisplayGamma);
	}

	ImGui::Separator(5.0f);

	if (ImGui::Hotkey("Menu Keybind##menu-show", &ShowKeybind)) 
	{
		Settings::SetSetting({ "Menu", "ShowKeybind" }, ShowKeybind);
	}

	ImGui::SameLine();

	if (ImGui::Button("Reset##showKeybind")) 
	{
		Settings::SetSetting({ "Menu", "ShowKeybind" }, ShowKeybind = VK_INSERT);
	}

	ImGui::SameLine();

	if (ImGui::Button("Debug Console##client-show-console")) 
	{
		Debug::CreateConsole();
	}
	
	ImGui::HelpMarker("Creates a console window that will display debug info from printf(). If you close the debug console, it will close Mirror's Edge too");
}

static void WorldTab() 
{
	auto world = Engine::GetWorld();
	if (!world) 
	{
		return;
	}

	ImGui::InputFloat("Time Dilation##world-time-dilation", &world->TimeDilation);
	ImGui::InputFloat("Gravity##world-gravity", &world->WorldGravityZ);

	if (LevelName.empty())
	{
		LevelName = world->GetMapName(false).c_str();
	}

	ImGui::Separator(5.0f);

	auto levels = world->StreamingLevels;
	if (ImGui::TreeNode("world##world-levels", "%ws (%d)", LevelName.c_str(), levels.Num()))
	{
		for (auto i = 0UL; i < levels.Num(); ++i) 
		{
			auto level = levels.GetByIndex(i);
			if (level) 
			{
				bool check = level->bShouldBeLoaded;
				auto label = level->PackageName.GetName();

				if (level->bHasLoadRequestPending || level->bHasUnloadRequestPending) 
				{
					ImGui::BeginDisabled();
					ImGui::Checkbox(label.c_str(), &check);
					ImGui::EndDisabled();
				} 
				else if (ImGui::Checkbox(label.c_str(), &check)) 
				{
					level->bShouldBeLoaded = level->bShouldBeVisible = check;
				}
			}
		}

		ImGui::TreePop();
	}
}

void Menu::AddTab(const char *name, MenuTabCallback callback) 
{
	Tabs.push_back({ name, callback });
}

void Menu::Hide() 
{
	Engine::BlockInput(ShowMenu = false);

	for (const auto& callback : menu.Callbacks) 
	{
		callback(false);
	}
}

void Menu::Show() 
{
	Engine::BlockInput(ShowMenu = true);

	for (const auto& callback : menu.Callbacks) 
	{
		callback(true);
	}
}

void Menu::OnVisibilityChange(MenuCallback callback)
{
	menu.Callbacks.push_back(callback);
}

bool Menu::Initialize() 
{
	ShowKeybind = Settings::GetSetting({ "Menu", "ShowKeybind" }, VK_INSERT);

	Engine::OnRenderScene(RenderMenu);

	Engine::OnInput([](unsigned int &msg, int keycode) 
	{
		if (!ShowMenu && msg == WM_KEYUP && keycode == ShowKeybind)
		{
			Show();
		}
	});

	Engine::OnSuperInput([](unsigned int &msg, int keycode) 
	{
		if (ShowMenu && msg == WM_KEYUP && (keycode == ShowKeybind || keycode == VK_ESCAPE))
		{
			Hide();
		}
	});

	Engine::OnPostLevelLoad([](const wchar_t *newLevelName) 
	{
		LevelName = newLevelName;
	});

	AddTab("Engine", EngineTab);
	AddTab("World", WorldTab);

	return true;
}