#pragma once
#include "../Util.h"

class ConfigurationPatcher {
private:
	static string loadXml();
public:
	static tstring getFilename();
	static pair<int64_t,string> getFileInfo();
	static void load();
};
