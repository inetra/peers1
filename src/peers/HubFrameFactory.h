#pragma once
class HubFrameFactory {
public:
	static void openWindow(const tstring& server
		, bool autoConnect
		, const tstring& rawOne = Util::emptyStringT
		, const tstring& rawTwo = Util::emptyStringT
		, const tstring& rawThree = Util::emptyStringT
		, const tstring& rawFour = Util::emptyStringT
		, const tstring& rawFive = Util::emptyStringT
		, int windowposx = 0, int windowposy = 0, int windowsizex = 0, int windowsizey = 0, int windowtype = 0, int chatusersplit = 0, bool userliststate = true,
		string sColumsOrder = Util::emptyString, string sColumsWidth = Util::emptyString, string sColumsVisible = Util::emptyString);
	static void closeAll(size_t thershold = 0);
	static void closeDisconnected();
	static void reconnectDisconnected();
	static void resortUsers();
	static void addDupeUsersToSummaryMenu(const int64_t &share, const string& ip);

	static bool activateAnyChat();
	static bool activateThisChat(MDIContainer::Window window);
	static bool activateHubWindow(const string& hubURL, bool activateChat);
};
