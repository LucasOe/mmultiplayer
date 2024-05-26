#include <fstream>
#include <Windows.h>

#include "settings.h"

static json settings;

static std::string GetSettingsPath() 
{
	return "mmultiplayer-settings.json";
}

void Settings::SetSetting(const std::vector<std::string> keys, const json value) 
{
	json* current = &settings;
	for (const auto& key : keys) 
	{
		if (current->find(key) == current->end()) 
		{
			(*current)[key] = json::object();
		}
		current = &((*current)[key]);
	}
	*current = value;

	Settings::Save();
}

json Settings::GetSetting(const std::vector<std::string> keys, const json defaultValue)
{
	json* current = &settings;

	for (size_t i = 0; i < keys.size(); ++i) 
	{
		if (current->find(keys[i]) == current->end()) 
		{
			if (i == keys.size() - 1) 
			{
				(*current)[keys[i]] = defaultValue;
			}
			else 
			{
				(*current)[keys[i]] = json::object();
			}
		}

		current = &((*current)[keys[i]]);
	}

	Settings::Save();
	return *current;
}

void Settings::Load() 
{
	bool reset = true;

	auto file = new std::ifstream(GetSettingsPath());
	if (file)
	{
		try 
		{
			settings = json::parse(*file);
			reset = false;
		} catch (json::parse_error e) {}

		file->close();
	}

	if (reset) 
	{
		Reset();
	}
}

void Settings::Reset() 
{
	settings = json::object();
	Settings::Save();
}

void Settings::Save() 
{
	std::ofstream file(GetSettingsPath(), std::ios::out);
	if (!file) 
	{
		printf("settings: failed to save %s\n", GetSettingsPath().c_str());
		return;
	}

	auto dump = settings.dump();
	file.write(dump.c_str(), dump.size());
	file.close();
}