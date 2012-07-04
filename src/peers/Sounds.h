#pragma once

class Sounds {
public:
	static string resolve(const string& filename);
	static void PlaySound(SettingsManager::StrSetting setting, bool beepAnyway = false);
};
