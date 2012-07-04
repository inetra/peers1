#include "stdinc.h"
#include "ConfigurationPatcher.h"
#include "../File.h"
#include "../../utils/SystemUtils.h"
#include "../../md5/MD5Digest.h"

tstring ConfigurationPatcher::getFilename() {
	tstring appDataFolder = SystemUtils::getAppDataFolder();
	if (!appDataFolder.empty()) {
		return appDataFolder + _T("conf\\configuration.xml");
	}
	return Util::emptyStringT;
}

pair<int64_t,string> ConfigurationPatcher::getFileInfo() {
	MD5Digest md5;
	string xml = loadXml();
	md5.update((const md5_byte_t*) xml.c_str(), xml.length());
	return make_pair(xml.length(), md5.digestAsString());
}

string ConfigurationPatcher::loadXml() {
	const tstring filename = getFilename();
	if (!filename.empty()) {
		try {
			return File(Text::fromT(filename), File::READ, File::OPEN).read();
		}
		catch (const FileException& e) {
			dcdebug("Error loading %s: %s\n", Text::fromT(filename).c_str(), e.getError().c_str());
		}
	}
	return Util::emptyString;
}

void ConfigurationPatcher::load() {
	SettingsManager::getInstance()->load(Text::fromT(getFilename()), false);
}
