/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(WIN_UTIL_H)
#define WIN_UTIL_H

#include "../client/User.h"
#include "../client/MerkleTree.h"

#include "resource.h"
#include "OMenu.h"

// Some utilities for handling HLS colors, taken from Jean-Michel LE FOL's codeproject
// article on WTL OfficeXP Menus
typedef DWORD HLSCOLOR;
#define HLS(h,l,s) ((HLSCOLOR)(((BYTE)(h)|((WORD)((BYTE)(l))<<8))|(((DWORD)(BYTE)(s))<<16)))
#define HLS_H(hls) ((BYTE)(hls))
#define HLS_L(hls) ((BYTE)(((WORD)(hls)) >> 8))
#define HLS_S(hls) ((BYTE)((hls)>>16))

HLSCOLOR RGB2HLS (COLORREF rgb);
COLORREF HLS2RGB (HLSCOLOR hls);

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S);

class FlatTabCtrl;
class UserCommand;

class WinUtil {
public:
	static CImageList fileImages;
	static int fileImageCount;
	static CImageList userImages;
    static CImageList flagImages;
    static int flagImageCount; // !SMT!-IP

        // !SMT!-UI search user by exact share size
        typedef HASH_MULTIMAP<uint64_t,UserPtr> ShareMap;
        typedef ShareMap::iterator ShareIter;
        //static ShareMap UsersShare;

	struct TextItem {
		WORD itemID;
		ResourceManager::Strings translatedString;
	};

	typedef HASH_MAP<tstring, int> ImageMap;
	typedef ImageMap::const_iterator ImageIter;
	static ImageMap fileIndexes;
	static HBRUSH bgBrush;
	static COLORREF textColor;
	static COLORREF bgColor;
	static HFONT font;
	static int fontHeight;
	static HFONT boldFont;
	static HFONT systemFont;
	static HFONT smallBoldFont;
	static CMenu mainMenu;
	static CMenu grantMenu;
        static CMenu speedMenu; // !SMT!-S
        static CMenu privateMenu; // !SMT!-PSW
        static OMenu userSummaryMenu; // !SMT!-UI
        static HICON banIconOnline; // !SMT!-UI
        static HICON banIconOffline; // !SMT!-UI
	static int dirIconIndex;
	static int dirMaskedIndex;
	static TStringList lastDirs;
	static HWND mainWnd;
	static HWND mdiClient;
	static FlatTabCtrl* tabCtrl;
        static tstring getChatHelp();
	static HHOOK hook;
	static tstring tth;
	static StringPairList initialDirs;	
	static tstring exceptioninfo;
	static DWORD helpCookie;	
	static bool isAppActive;
	static CHARFORMAT2 m_TextStyleTimestamp;
	static CHARFORMAT2 m_ChatTextGeneral;
	static CHARFORMAT2 m_TextStyleMyNick;
	static CHARFORMAT2 m_ChatTextMyOwn;
	static CHARFORMAT2 m_ChatTextServer;
	static CHARFORMAT2 m_ChatTextSystem;
	static CHARFORMAT2 m_TextStyleBold;
	static CHARFORMAT2 m_TextStyleFavUsers;
	static CHARFORMAT2 m_TextStyleOPs;
	static CHARFORMAT2 m_TextStyleURL;
	static CHARFORMAT2 m_ChatTextPrivate;
	static CHARFORMAT2 m_ChatTextLog;
	static bool mutesounds;
	static DWORD comCtlVersion;	

	static void init(HWND hWnd);
	static void uninit();

	static void initColors();
	static void reLoadImages(); // User Icon Begin / User Icon End

        static bool getUserColor(const UserPtr& aUser, COLORREF &fg, COLORREF &bg, Client *hub = 0); // !SMT!-UI

	static string getAppName() {
		TCHAR buf[MAX_PATH+1];
		DWORD x = GetModuleFileName(NULL, buf, MAX_PATH);
		return Text::fromT(tstring(buf, x));
	}

        static void clearSummaryMenu(); // !SMT!-UI
        static void decodeFont(const tstring& setting, LOGFONT &dest);

	static void addInitalDir(const UserPtr& user, string dir) {
		// Clear out previos initial dirs, just in case
		/// @todo clean up
		getInitialDir(user);
		while(initialDirs.size() > 30) {
			initialDirs.erase(initialDirs.begin());
		}
		initialDirs.push_back(make_pair(user->getCID().toBase32(), dir));
	}

	static string getInitialDir(const UserPtr& user) {
		for(StringPairIter i = initialDirs.begin(); i != initialDirs.end(); ++i) {
			if(i->first == user->getCID().toBase32()) {
				string dir = i->second;
				initialDirs.erase(i);
				return dir;
			}
		}
		return Util::emptyString;
	}

	static bool getVersionInfo(OSVERSIONINFOEX& ver);

	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @return True if the command was processed, false otherwise.
	 */
	static bool checkCommand(const tstring& cmd, const tstring& param, tstring& message, tstring& status);

	static int getTextWidth(const tstring& str, HWND hWnd) {
		HDC dc = ::GetDC(hWnd);
		int sz = getTextWidth(str, dc);
		::ReleaseDC(mainWnd, dc);
		return sz;
	}
	static int getTextWidth(const tstring& str, HDC dc) {
		SIZE sz = { 0, 0 };
		::GetTextExtentPoint32(dc, str.c_str(), str.length(), &sz);
		return sz.cx;		
	}

	static int getTextHeight(HWND wnd, HFONT fnt) {
		HDC dc = ::GetDC(wnd);
		int h = getTextHeight(dc, fnt);
		::ReleaseDC(wnd, dc);
		return h;
	}

	static int getTextHeight(HDC dc, HFONT fnt) {
		HGDIOBJ old = ::SelectObject(dc, fnt);
		int h = getTextHeight(dc);
		::SelectObject(dc, old);
		return h;
	}

	static int getTextHeight(HDC dc) {
		TEXTMETRIC tm;
		::GetTextMetrics(dc, &tm);
		return tm.tmHeight;
	}

	static void setClipboard(const tstring& str);

	static void addLastDir(const tstring& dir) {
		if(find(lastDirs.begin(), lastDirs.end(), dir) != lastDirs.end()) {
			return;
		}
		if(lastDirs.size() == 10) {
			lastDirs.erase(lastDirs.begin());
		}
		lastDirs.push_back(dir);
	}
	
        static void UnlinkStaticMenus(CMenu &menu); // !SMT!-UI

	static tstring encodeFont(LOGFONT const& font);
	
	static bool browseFile(tstring& target, HWND owner = NULL, bool save = true, const tstring& initialDir = Util::emptyStringT, const TCHAR* types = NULL, const TCHAR* defExt = NULL);
	static bool browseDirectory(tstring& target, HWND owner = NULL);

	// Hash related
#ifdef PPA_INCLUDE_BITZI_LOOKUP
 	static void bitziLink(const TTHValue& /*aHash*/);
#endif
	static void copyMagnet(const TTHValue& /*aHash*/, const tstring& /*aFile*/, int64_t);
        static string getMagnet(const TTHValue& aHash, const tstring& aFile, int64_t aSize);
        static string getWebMagnet(const TTHValue& aHash, const tstring& aFile, int64_t aSize); // !SMT!-UI
	static void searchHash(const TTHValue& /*aHash*/);

	// URL related
	static void registerDchubHandler();
	static void registerADChubHandler();
	static void registerMagnetHandler();
	static void allowUserTOSSetting();
	static void unRegisterDchubHandler();
	static void unRegisterADChubHandler();
	static void unRegisterMagnetHandler();
	static void parseDchubUrl(const tstring& /*aUrl*/);
	static void parseADChubUrl(const tstring& /*aUrl*/);
	static void parseMagnetUri(const tstring& /*aUrl*/, bool aOverride = false);
	static bool parseDBLClick(const tstring& /*aString*/, string::size_type start, string::size_type end);
	static bool urlDcADCRegistered;
	static bool urlMagnetRegistered;
	static int textUnderCursor(POINT p, CEdit& ctrl, tstring& x);
	static void openLink(const tstring& url);
	static bool handleLink(const tstring& url);
	static void openFile(const tstring& file) {
		::ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	static void openFolder(const tstring& file);

	static int getIconIndex(const tstring& aFileName);

	static int getDirIconIndex() { return dirIconIndex; }
	static int getDirMaskedIndex() { return dirMaskedIndex; }
	
	static double toBytes(TCHAR* aSize);
	
	static int getOsMajor();
	static int getOsMinor();
	
	//returns the position where the context menu should be
	//opened if it was invoked from the keyboard.
	//aPt is relative to the screen not the control.
	static void getContextMenuPos(CListViewCtrl& aList, POINT& aPt);
	static void getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt);
	static void getContextMenuPos(CEdit& aEdit,			POINT& aPt);
	
	static bool getUCParams(HWND parent, const UserCommand& cmd, StringMap& sm) throw();

	/*static tstring getNicks(const CID& cid) throw();
	static tstring getNicks(const UserPtr& u) {
		if(u->isSet(User::NMDC)) {
			return Text::toT(u->getFirstNick());
		} else  {
			return getNicks(u->getCID());
		}
	}*/
	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid) throw();
	static pair<tstring, bool> getHubNames(const UserPtr& u) { return getHubNames(u->getCID()); }

	static void splitTokens(int8_t* array, const string& tokens, int maxItems = -1) throw();
	static void splitTokens(int* array, const string& tokens, int maxItems = -1) throw();
	static void saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
                SettingsManager::StrSetting widths, int n, int* indexes, int* sizes) throw(); // !SMT!-UI todo: disable - this routine does not save column visibility

	static bool isShift() { return (GetKeyState(VK_SHIFT) & 0x8000) > 0; }
	static bool isAlt() { return (GetKeyState(VK_MENU) & 0x8000) > 0; }
	static bool isCtrl() { return (GetKeyState(VK_CONTROL) & 0x8000) > 0; }

	static tstring escapeMenu(tstring str) { 
		string::size_type i = 0;
		while( (i = str.find(_T('&'), i)) != string::npos) {
			str.insert(str.begin()+i, 1, _T('&'));
			i += 2;
		}
		return str;
	}
	template<class T> static HWND hiddenCreateEx(T& p) throw() {
		HWND active = (HWND)::SendMessage(mdiClient, WM_MDIGETACTIVE, 0, 0);
		::LockWindowUpdate(mdiClient);
		HWND ret = p.CreateEx(mdiClient);
		if(active && ::IsWindow(active))
			::SendMessage(mdiClient, WM_MDIACTIVATE, (WPARAM)active, 0);
		::LockWindowUpdate(0);
		return ret;
	}
	template<class T> static HWND hiddenCreateEx(T* p) throw() {
		return hiddenCreateEx(*p);
	}
	
	static void translate(HWND page, TextItem* textItems) 
	{
		if (textItems != NULL) {
			for(int i = 0; textItems[i].itemID != 0; i++) {
				::SetDlgItemText(page, textItems[i].itemID,
					Text::toT(ResourceManager::getInstance()->getString(textItems[i].translatedString)).c_str());
			}
		}
	}

	struct tbIDImage {
		int id, image;
		bool check;
		ResourceManager::Strings tooltip;
	};
	static tbIDImage ToolbarButtons[];

	static void ClearPreviewMenu(OMenu &previewMenu);
	static int SetupPreviewMenu(CMenu &previewMenu, string extension);
	static void RunPreviewCommand(unsigned int index, string target);
	static string formatTime(long rest);
	static uint8_t getImage(const Identity& u);
    static int getFlagImage(string& country, bool fullname = false); // !SMT!-IP
	static string generateStats();
//[-]PPA	static string disableCzChars(string message);
	static bool shutDown(int action);
	static int getFirstSelectedIndex(CListViewCtrl& list);
	static int setButtonPressed(int nID, bool bPressed = true);
	static string getWMPSpam(HWND playerWnd = NULL);
	static string getItunesSpam(HWND playerWnd = NULL);
	static string getMPCSpam();
	static string getWinampSpam(HWND playerWnd = NULL);
/*
static string getActionStr(int raw) {
		switch(raw) {
			case 0:	return STRING(NO_ACTION); break;
			case 1:	return SETTING(RAW1_TEXT); break;
			case 2:	return SETTING(RAW2_TEXT); break;
			case 3:	return SETTING(RAW3_TEXT); break;
			case 4:	return SETTING(RAW4_TEXT); break;
			case 5:	return SETTING(RAW5_TEXT); break;
			default: return STRING(NO_ACTION); break;
		}
        }
*/
        // !SMT!-UI
        static COLORREF blendColors(COLORREF a, COLORREF b) {
                return ((uint32_t)a & 0xFEFEFE)/2 + ((uint32_t)b & 0xFEFEFE)/2;
        }

// FDM extension
static string findNickInTString(const tstring aLine) {
		// set to 0, to fix warning C4701
		tstring::size_type i = 0;
		tstring::size_type j = 0;

		//Check For <Nick>
		if (((i = aLine.find_first_of('<')) != string::npos) && ((j = aLine.find_first_of('>')) != string::npos && j > i))
			return Text::fromT(aLine.substr(i + 1, j - i - 1).c_str());
		//Check for * Nick
		if ((aLine.find(_T("***")) == string::npos) && ((i = Util::findSubString(Text::fromT(aLine).c_str(), "* ")) != string::npos) && ((j = aLine.find(' ', 2)) != string::npos && j > i))
			return Text::fromT(aLine.substr(i + 2, j - i - 2).c_str());
		return Util::emptyString;
	}

	static string getIpCountryForChat(string ip, bool ts) {
                if (!ip.empty() /* && Util::fileExists(Util::getConfigPath() + "GeoIPCountryWhois.csv") !SMT!-IP */ && BOOLSETTING(COUNTRY_IN_CHAT) && BOOLSETTING(IP_IN_CHAT)) {
			return ts ? " | " + ip + " | " + Util::getIpCountry(ip) : " " + ip + " | " + Util::getIpCountry(ip);
                } else if (!ip.empty() /* && Util::fileExists(Util::getConfigPath() + "GeoIPCountryWhois.csv") !SMT!-IP */ && BOOLSETTING(COUNTRY_IN_CHAT)) {
			return ts ? " | " + Util::getIpCountry(ip) : Util::getIpCountry(ip);
		} else if (!ip.empty() && BOOLSETTING(IP_IN_CHAT)) {
			return ts ? " | " + ip + " " : " " + ip + " ";
		}
		return Util::emptyString;
	}
// FDM extension

        static void ShowBalloonTip(LPCTSTR szMsg, LPCTSTR szTitle, DWORD dwInfoFlags=NIIF_INFO);
        static void showPrivateMessageTrayIcon();
        static bool isAppMinimized();
        static HWND MDIGetActive();
        static void MDIActivate(HWND hWnd);
		static void showVideo(const string& path);
		static void stopVideo();
		static void showVideoSplash();
		static bool isVideo(const string &fileName);

private:
	static int CALLBACK browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData);

};

#endif // !defined(WIN_UTIL_H)

/**
 * @file
 * $Id: WinUtil.h,v 1.6 2008/03/10 07:23:51 alexey Exp $
 */
