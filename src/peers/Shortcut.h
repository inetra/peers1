#pragma once

class Shortcut {
public:
	static void createDesktopShortcut(const tstring& fileName, const tstring& displayName);
	static void createInternetShortcut(const tstring& url, const tstring& displayName, const tstring& icon, int iconIndex);
};
