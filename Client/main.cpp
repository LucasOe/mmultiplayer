#include <windows.h>

#include "debug.h"
#include "menu.h"
#include "settings.h"
#include "addon.h"

#include "addons/dolly.h"
#include "addons/client.h"
#include "addons/trainer.h"
#include "addons/misc.h"
#include "addons/chaos/chaos.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        Debug::Initialize();

        while (!GetModuleHandle(L"d3d9.dll")) {
            Sleep(100);
        }

        Settings::Load();

        Addon *addons[] = { new Client(), new Trainer(), new Dolly(), new Misc(), new Chaos() };

        if (!Engine::Initialize()) {
            MessageBoxA(nullptr, "Failed to initialize engine", "Fatal", 0);
            goto CLEANUP;
        }

        if (!Menu::Initialize()) {
            MessageBoxA(nullptr, "Failed to initialize menu", "Fatal", 0);
            goto CLEANUP;
        }
		
        for (auto &addon : addons) {
            if (!addon->Initialize()) {
                MessageBoxA(nullptr, ("Failed to initialize \"" + addon->GetName() + "\"").c_str(), "Fatal", 0);
            }
        }
	
        CLEANUP:
            for (const auto addon: addons) {
                delete addon;
            }
    }
	return TRUE;
}