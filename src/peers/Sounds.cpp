#include "stdafx.h"
#include "../client/SettingsManager.h"
#include "../client/Text.h"
#include "Sounds.h"

string Sounds::resolve(const string& filename) {
	if (!filename.empty() && filename[0] == '.') {
		return Util::getSystemPath() + filename.substr(1);
	}
	return filename;
}

void Sounds::PlaySound(SettingsManager::StrSetting setting, bool beepAnyway) {
	if (BOOLSETTING(SOUNDS_DISABLED)) {
		return;
	}
	string filename = SettingsManager::getInstance()->get(setting, true);
	if (filename.empty()) {
		if (beepAnyway) {
			MessageBeep(MB_OK);
		}
	}
	else {
		::PlaySound(Text::toT(resolve(filename)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}
}
