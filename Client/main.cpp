#include <windows.h>

#include "debug.h"
#include "menu.h"
#include "settings.h"
#include "addon.h"

#define GAMES_ADDON_SUPPORTED 0
#define MISC_ADDON_SUPPORTED 0
#define CHAOS_ADDON_SUPPORTED 0

#include "addons/dolly.h"
#include "addons/client.h"
#include "addons/trainer.h"

#if MISC_ADDON_SUPPORTED
#include "addons/misc.h"
#endif

#if CHAOS_ADDON_SUPPORTED
#include "addons/chaos/chaos.h"
#endif

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) 
{
    if (reason == DLL_PROCESS_ATTACH) 
	{
		Debug::Initialize();

		while (!GetModuleHandle(L"d3d9.dll")) 
		{
			Sleep(100);
		}

		Settings::Load();

		Addon *addons[] = 
		{ 
			new Client(), 
			new Trainer(), 
			new Dolly(), 

#if MISC_ADDON_SUPPORTED
			new Misc(),
#endif

#if CHAOS_ADDON_SUPPORTED
			new Chaos(), 
#endif
		};

		if (!Engine::Initialize()) 
		{
			MessageBoxA(0, "Failed to initialize engine", "Fatal", 0);
			return TRUE;
		}

		if (!Menu::Initialize()) 
		{
			MessageBoxA(0, "Failed to initialize menu", "Fatal", 0);
			return TRUE;
		}
		
		for (auto &addon : addons) 
		{
			if (!addon->Initialize()) 
			{
				MessageBoxA(0, ("Failed to initialize \"" + addon->GetName() + "\"").c_str(), "Fatal", 0);
			}
		}
	}
	
	return TRUE;
}