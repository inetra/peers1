#pragma once

class SystemUtils {
public:
	static string describeError(int aError);
	static tstring getUserAppDataFolder();
	static tstring getAppDataFolder();
	static tstring getProfileFolder();
	static tstring getDocumentFolder();
};
