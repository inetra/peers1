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

#include "stdafx.h"

#define COMPILE_MULTIMON_STUBS 1
#include <MultiMon.h>
#include <psapi.h>
#include <powrprof.h>

#include "../peers/SearchFrmFactory.h"
#include "../peers/HubFrameFactory.h"
#include "MainFrm.h"

#include "../client/ShareManager.h"
#include "../client/UploadManager.h"
#include "../client/HashManager.h"
#include "../client/AssocVector.h"
#include "../client/IgnoreManager.h"
#ifdef _DEBUG
#include "../client/TraceManager.h"
#endif

#include "MagnetDlg.h"
#include "winamp.h"
#include "WMPlayerRemoteApi.h"
#include "iTunesCOMInterface.h"
#include <control.h>
#include <ddraw.h>
#include <strmif.h>	// error with missing ddraw.h, get it from MS DirectX SDK
#include "LineDlg.h"
#include "BarShader.h"
#include "../peers/PeersVersion.h"

WinUtil::ImageMap WinUtil::fileIndexes;
int WinUtil::fileImageCount;
HBRUSH WinUtil::bgBrush = NULL;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
HFONT WinUtil::font = NULL;
int WinUtil::fontHeight = 0;
HFONT WinUtil::boldFont = NULL;
HFONT WinUtil::systemFont = NULL;
HFONT WinUtil::smallBoldFont = NULL;
CMenu WinUtil::mainMenu;
CMenu WinUtil::grantMenu;
CMenu WinUtil::speedMenu; // !SMT!-S
CMenu WinUtil::privateMenu; // !SMT!-PSW
OMenu WinUtil::userSummaryMenu; // !SMT!-UI
HICON WinUtil::banIconOnline; // !SMT!-UI
HICON WinUtil::banIconOffline; // !SMT!-UI
//static WinUtil::ShareMap WinUtil::UsersShare; // !SMT!-UI
CImageList WinUtil::fileImages;
CImageList WinUtil::userImages;
CImageList WinUtil::flagImages;
int WinUtil::flagImageCount; // !SMT!-IP
int WinUtil::dirIconIndex = 0;
int WinUtil::dirMaskedIndex = 0;
TStringList WinUtil::lastDirs;
HWND WinUtil::mainWnd = NULL;
HWND WinUtil::mdiClient = NULL;
FlatTabCtrl* WinUtil::tabCtrl = NULL;
HHOOK WinUtil::hook = NULL;
StringPairList WinUtil::initialDirs;
tstring WinUtil::exceptioninfo;
bool WinUtil::urlDcADCRegistered = false;
bool WinUtil::urlMagnetRegistered = false;
bool WinUtil::isAppActive = false;
DWORD WinUtil::comCtlVersion = 0;
CHARFORMAT2 WinUtil::m_TextStyleTimestamp;
CHARFORMAT2 WinUtil::m_ChatTextGeneral;
CHARFORMAT2 WinUtil::m_TextStyleMyNick;
CHARFORMAT2 WinUtil::m_ChatTextMyOwn;
CHARFORMAT2 WinUtil::m_ChatTextServer;
CHARFORMAT2 WinUtil::m_ChatTextSystem;
CHARFORMAT2 WinUtil::m_TextStyleBold;
CHARFORMAT2 WinUtil::m_TextStyleFavUsers;
CHARFORMAT2 WinUtil::m_TextStyleOPs;
CHARFORMAT2 WinUtil::m_TextStyleURL;
CHARFORMAT2 WinUtil::m_ChatTextPrivate;
CHARFORMAT2 WinUtil::m_ChatTextLog;

WinUtil::tbIDImage WinUtil::ToolbarButtons[] = {
	{ID_FILE_CONNECT, 0, true, ResourceManager::MENU_PUBLIC_HUBS},
	{ID_FILE_RECONNECT, 1, false, ResourceManager::MENU_RECONNECT},
	{IDC_FOLLOW, 2, false, ResourceManager::MENU_FOLLOW_REDIRECT},
	{IDC_FAVORITES, 3, true, ResourceManager::MENU_FAVORITE_HUBS},
	{IDC_FAVUSERS, 4, true, ResourceManager::MENU_FAVORITE_USERS},
	{IDC_RECENTS, 5, true, ResourceManager::MENU_FILE_RECENT_HUBS},
	{IDC_QUEUE, 6, true, ResourceManager::MENU_DOWNLOAD_QUEUE},
	{IDC_FINISHED, 7, true, ResourceManager::MENU_FINISHED_DOWNLOADS},
	{IDC_UPLOAD_QUEUE, 8, true, ResourceManager::MENU_WAITING_USERS},
	{IDC_FINISHED_UL, 9, true, ResourceManager::MENU_FINISHED_UPLOADS},
	{ID_FILE_SEARCH, 10, false, ResourceManager::MENU_SEARCH},
	{IDC_FILE_ADL_SEARCH, 11, true, ResourceManager::MENU_ADL_SEARCH},
	{IDC_SEARCH_SPY, 12, true, ResourceManager::MENU_SEARCH_SPY},
	{IDC_NET_STATS, 13, true, ResourceManager::NETWORK_STATISTICS},
	{IDC_OPEN_FILE_LIST, 14, false, ResourceManager::MENU_OPEN_FILE_LIST},
	{ID_FILE_SETTINGS, 15, false, ResourceManager::MENU_SETTINGS},
	{IDC_NOTEPAD, 16, true, ResourceManager::MENU_NOTEPAD},
	{IDC_AWAY, 17, true, ResourceManager::AWAY},
	{IDC_SHUTDOWN, 18, true, ResourceManager::SHUTDOWN},
	{IDC_LIMITER, 19, true, ResourceManager::SETCZDC_ENABLE_LIMITING},
	{IDC_DISABLE_SOUNDS, 21, true, ResourceManager::DISABLE_SOUNDS},
	{IDC_OPEN_DOWNLOADS, 22, false, ResourceManager::MENU_OPEN_DOWNLOADS_DIR},
	{IDC_REFRESH_FILE_LIST, 23, false, ResourceManager::MENU_REFRESH_FILE_LIST},
	{ID_TOGGLE_TOOLBAR, 24, true, ResourceManager::TOGGLE_TOOLBAR},
	{ID_FILE_QUICK_CONNECT, 25, false, ResourceManager::MENU_QUICK_CONNECT},
	{0, 0, false, ResourceManager::MENU_NOTEPAD}
};
#ifdef PPA_USE_COUNTRY
static const char* CountryNames[] = { "ANDORRA", "UNITED ARAB EMIRATES", "AFGHANISTAN", "ANTIGUA AND BARBUDA", 
"ANGUILLA", "ALBANIA", "ARMENIA", "NETHERLANDS ANTILLES", "ANGOLA", "ANTARCTICA", "ARGENTINA", "AMERICAN SAMOA", 
"AUSTRIA", "AUSTRALIA", "ARUBA", "ALAND", "AZERBAIJAN", "BOSNIA AND HERZEGOVINA", "BARBADOS", "BANGLADESH", 
"BELGIUM", "BURKINA FASO", "BULGARIA", "BAHRAIN", "BURUNDI", "BENIN", "BERMUDA", "BRUNEI DARUSSALAM", "BOLIVIA", 
"BRAZIL", "BAHAMAS", "BHUTAN", "BOUVET ISLAND", "BOTSWANA", "BELARUS", "BELIZE", "CANADA", "COCOS ISLANDS", 
"THE DEMOCRATIC REPUBLIC OF THE CONGO", "CENTRAL AFRICAN REPUBLIC", "CONGO", "COTE D'IVOIRE", "COOK ISLANDS", 
"CHILE", "CAMEROON", "CHINA", "COLOMBIA", "COSTA RICA", "SERBIA AND MONTENEGRO", "CUBA", "CAPE VERDE", 
"CHRISTMAS ISLAND", "CYPRUS", "CZECH REPUBLIC", "GERMANY", "DJIBOUTI", "DENMARK", "DOMINICA", "DOMINICAN REPUBLIC", 
"ALGERIA", "ECUADOR", "ESTONIA", "EGYPT", "WESTERN SAHARA", "ERITREA", "SPAIN", "ETHIOPIA", "FINLAND", "FIJI", 
"FALKLAND ISLANDS", "MICRONESIA", "FAROE ISLANDS", "FRANCE", "GABON", "UNITED KINGDOM", "GRENADA", "GEORGIA", 
"FRENCH GUIANA", "GHANA", "GIBRALTAR", "GREENLAND", "GAMBIA", "GUINEA", "GUADELOUPE", "EQUATORIAL GUINEA", 
"GREECE", "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", "GUATEMALA", "GUAM", "GUINEA-BISSAU", "GUYANA", 
"HONG KONG", "HEARD ISLAND AND MCDONALD ISLANDS", "HONDURAS", "CROATIA", "HAITI", "HUNGARY", "SWITZERLAND", 
"INDONESIA", "IRELAND", "ISRAEL", "INDIA", "BRITISH INDIAN OCEAN TERRITORY", "IRAQ", "IRAN", "ICELAND", 
"ITALY", "JAMAICA", "JORDAN", "JAPAN", "KENYA", "KYRGYZSTAN", "CAMBODIA", "KIRIBATI", "COMOROS", 
"SAINT KITTS AND NEVIS", "DEMOCRATIC PEOPLE'S REPUBLIC OF KOREA", "SOUTH KOREA", "KUWAIT", "CAYMAN ISLANDS", 
"KAZAKHSTAN", "LAO PEOPLE'S DEMOCRATIC REPUBLIC", "LEBANON", "SAINT LUCIA", "LIECHTENSTEIN", "SRI LANKA", 
"LIBERIA", "LESOTHO", "LITHUANIA", "LUXEMBOURG", "LATVIA", "LIBYAN ARAB JAMAHIRIYA", "MOROCCO", "MONACO", 
"MOLDOVA", "MADAGASCAR", "MARSHALL ISLANDS", "MACEDONIA", "MALI", "MYANMAR", "MONGOLIA", "MACAO", 
"NORTHERN MARIANA ISLANDS", "MARTINIQUE", "MAURITANIA", "MONTSERRAT", "MALTA", "MAURITIUS", "MALDIVES", 
"MALAWI", "MEXICO", "MALAYSIA", "MOZAMBIQUE", "NAMIBIA", "NEW CALEDONIA", "NIGER", "NORFOLK ISLAND", 
"NIGERIA", "NICARAGUA", "NETHERLANDS", "NORWAY", "NEPAL", "NAURU", "NIUE", "NEW ZEALAND", "OMAN", "PANAMA", 
"PERU", "FRENCH POLYNESIA", "PAPUA NEW GUINEA", "PHILIPPINES", "PAKISTAN", "POLAND", "SAINT PIERRE AND MIQUELON", 
"PITCAIRN", "PUERTO RICO", "PALESTINIAN TERRITORY", "PORTUGAL", "PALAU", "PARAGUAY", "QATAR", "REUNION", 
"ROMANIA", "RUSSIAN FEDERATION", "RWANDA", "SAUDI ARABIA", "SOLOMON ISLANDS", "SEYCHELLES", "SUDAN", 
"SWEDEN", "SINGAPORE", "SAINT HELENA", "SLOVENIA", "SVALBARD AND JAN MAYEN", "SLOVAKIA", "SIERRA LEONE", 
"SAN MARINO", "SENEGAL", "SOMALIA", "SURINAME", "SAO TOME AND PRINCIPE", "EL SALVADOR", "SYRIAN ARAB REPUBLIC", 
"SWAZILAND", "TURKS AND CAICOS ISLANDS", "CHAD", "FRENCH SOUTHERN TERRITORIES", "TOGO", "THAILAND", "TAJIKISTAN", 
"TOKELAU", "TIMOR-LESTE", "TURKMENISTAN", "TUNISIA", "TONGA", "TURKEY", "TRINIDAD AND TOBAGO", "TUVALU", "TAIWAN", 
"TANZANIA", "UKRAINE", "UGANDA", "UNITED STATES MINOR OUTLYING ISLANDS", "UNITED STATES", "URUGUAY", "UZBEKISTAN", 
"VATICAN", "SAINT VINCENT AND THE GRENADINES", "VENEZUELA", "BRITISH VIRGIN ISLANDS", "U.S. VIRGIN ISLANDS", 
"VIET NAM", "VANUATU", "WALLIS AND FUTUNA", "SAMOA", "YEMEN", "MAYOTTE", "YUGOSLAVIA", "SOUTH AFRICA", "ZAMBIA", 
"ZIMBABWE", "EUROPEAN UNION" };

static const char* CountryCodes[] = { "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AQ", "AR", "AS", 
"AT", "AU", "AW", "AX", "AZ", "BA", "BB", "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BM", "BN", "BO", "BR", 
"BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", "CD", "CF", "CG", "CI", "CK", "CL", "CM", "CN", "CO", "CR", 
"CS", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ", "EC", "EE", "EG", "EH", "ER", "ES", 
"ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB", "GD", "GE", "GF", "GH", "GI", "GL", "GM", "GN", "GP", 
"GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT", "HU", "CH", "ID", "IE", "IL", "IN", 
"IO", "IQ", "IR", "IS", "IT", "JM", "JO", "JP", "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", 
"KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", "LU", "LV", "LY", "MA", "MC", "MD", "MG", "MH", "MK", 
"ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", 
"NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", 
"PN", "PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RU", "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", 
"SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "ST", "SV", "SY", "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", 
"TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", "UM", "US", "UY", "UZ", "VA", "VC", 
"VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "YU", "ZA", "ZM", "ZW", "EU" };
#endif

HLSCOLOR RGB2HLS (COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff  = float(maxval) - float(minval);
	float msum   = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f; 

	if ( maxval != minval ) { 
		float rnorm = (maxval - GetRValue(rgb)  ) / mdiff;      
		float gnorm = (maxval - GetGValue(rgb)) / mdiff;
		float bnorm = (maxval - GetBValue(rgb) ) / mdiff;   

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if (GetRValue(rgb) == maxval) hue = 60.0f * (6.0f + bnorm - gnorm);
		if (GetGValue(rgb) == maxval) hue = 60.0f * (2.0f + rnorm - bnorm);
		if (GetBValue(rgb) == maxval) hue = 60.0f * (4.0f + gnorm - rnorm);
		if (hue > 360.0f) hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static BYTE _ToRGB (float rm1, float rm2, float rh) {
	if      (rh > 360.0f) rh -= 360.0f;
	else if (rh <   0.0f) rh += 360.0f;

	if      (rh <  60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;   
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;      

	return (BYTE)(rm1 * 255);
}

COLORREF HLS2RGB (HLSCOLOR hls) {
	float hue        = ((int)HLS_H(hls)*360)/255.0f;
	float luminance  = HLS_L(hls)/255.0f;
	float saturation = HLS_S(hls)/255.0f;

	if ( EqualD(saturation,0) ) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if ( luminance <= 0.5f ) rm2 = luminance + luminance * saturation;  
	else                     rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;   
	BYTE red   = _ToRGB (rm1, rm2, hue + 120.0f);   
	BYTE green = _ToRGB (rm1, rm2, hue);
	BYTE blue  = _ToRGB (rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS (rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if ( percent_L > 0 ) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if ( percent_L < 0 )	{
		l = BYTE((l * (100+percent_L)) / 100);
	}
	if ( percent_S > 0 ) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if ( percent_S < 0 ) {
		s = BYTE((s * (100+percent_S)) / 100);
	}
	return HLS2RGB (HLS(h, l, s));
}

// !SMT!-UI
bool WinUtil::getUserColor(const UserPtr& aUser, COLORREF &fg, COLORREF &bg, Client *hub)
{
	bool isOp = false;
	bool isPassive = false;
	{
     ClientManager::LockInstance l_lockInstance;
 	 OnlineUser* ou = hub? hub->getUser(aUser) : ClientManager::getInstance()->getOnlineUser(aUser);
	 isOp = ou && ou->getIdentity().isOp();
	 isPassive = ou && !ou->getIdentity().isTcpActive();
	}
	if (FavoriteManager::getInstance()->isFavoriteUser(aUser)) {
			fg = SETTING(FAVORITE_COLOR);
	} else if (UploadManager::getInstance()->getReservedSlotTime(aUser)) {
		fg = SETTING(RESERVED_SLOT_COLOR);
	} else if (IgnoreManager::getInstance()->isIgnored(aUser)) {
		fg = SETTING(IGNORED_COLOR);
	} else if(aUser->isSet(User::FIREBALL)) {
		fg = SETTING(FIREBALL_COLOR);
	} else if(aUser->isSet(User::SERVER)) {
		fg = SETTING(SERVER_COLOR);
	} else if(isOp) {
		fg = SETTING(OP_COLOR);
	} else if(isPassive) {
		fg = SETTING(PASIVE_COLOR);
	} else {
		fg = SETTING(NORMAL_COLOUR);
	}
	if (aUser->hasAutoBan())
		bg = SETTING(BAN_COLOR);
	return true;
} 

// !SMT!-UI
void WinUtil::UnlinkStaticMenus(CMenu &menu)
{
        MENUITEMINFO mif = { sizeof MENUITEMINFO };
        mif.fMask = MIIM_SUBMENU;
        for (int i = menu.GetMenuItemCount(); i; i--) {
                menu.GetMenuItemInfo(i-1, true, &mif);
                if (mif.hSubMenu == userSummaryMenu.m_hMenu || mif.hSubMenu == grantMenu.m_hMenu ||
                      mif.hSubMenu == speedMenu.m_hMenu || mif.hSubMenu == privateMenu.m_hMenu)
                        menu.RemoveMenu(i-1, MF_BYPOSITION);
        }
}

// !SMT!-UI
void WinUtil::clearSummaryMenu() {
        for (int i = userSummaryMenu.GetMenuItemCount(); i; i--)
                userSummaryMenu.RemoveMenu(0, MF_BYPOSITION);
}

bool WinUtil::getVersionInfo(OSVERSIONINFOEX& ver) {
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
			return false;
		}
	}
	return true;
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if(code == HC_ACTION) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			if(lParam & 0x80000000) {
				WinUtil::tabCtrl->endSwitch();
			} else {
				WinUtil::tabCtrl->startSwitch();
			}
		}
	}
	return CallNextHookEx(WinUtil::hook, code, wParam, lParam);
}

void WinUtil::reLoadImages(){
	userImages.Destroy();
	if(SETTING(USERLIST_IMAGE).empty())
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else
		userImages.CreateFromImage(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
}

void WinUtil::init(HWND hWnd) {
	mainWnd = hWnd;
        const bool advancedMenu = BOOLSETTING(MENU_ADVANCED);

	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CTSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, IDC_OPEN_MY_LIST, CTSTRING(MENU_OPEN_OWN_LIST));
	//file.AppendMenu(MF_STRING, IDC_MATCH_ALL, CTSTRING(MENU_OPEN_MATCH_ALL));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_FILELIST_ADD_FILE, CTSTRING(MENU_FILELIST_ADD_FILE));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_QUICK_CONNECT, CTSTRING(MENU_QUICK_CONNECT));
	file.AppendMenu(MF_STRING, IDC_FOLLOW, CTSTRING(MENU_FOLLOW_REDIRECT));
	file.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	file.AppendMenu(MF_SEPARATOR);
	//file.AppendMenu(MF_STRING, ID_FILE_SETTINGS, CTSTRING(MENU_SETTINGS));
	//file.AppendMenu(MF_STRING, ID_GET_TTH, CTSTRING(MENU_TTH));
	//file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)file, CTSTRING(MENU_FILE));

	CMenuHandle view;
	view.CreatePopupMenu();

//[-] PPA & <Squork>	view.AppendMenu(MF_STRING, ID_FILE_CONNECT, CTSTRING(MENU_PUBLIC_HUBS));
//[-] PPA & <Squork>	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_PEERS_TOOLBAR_CHAT, CTSTRING(MENU_VIEW_CHAT));
	view.AppendMenu(MF_STRING, IDC_FAVUSERS, CTSTRING(MENU_FAVORITE_USERS));
	//view.AppendMenu(MF_STRING, IDC_RECENTS, CTSTRING(MENU_FILE_RECENT_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_QUEUE, CTSTRING(MENU_DOWNLOAD_QUEUE));
	view.AppendMenu(MF_STRING, IDC_FINISHED, CTSTRING(MENU_FINISHED_DOWNLOADS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_UPLOAD_QUEUE, CTSTRING(MENU_WAITING_USERS));
	view.AppendMenu(MF_STRING, IDC_FINISHED_UL, CTSTRING(MENU_FINISHED_UPLOADS));
	//view.AppendMenu(MF_STRING, ID_FILE_SEARCH, CTSTRING(MENU_SEARCH));
	//view.AppendMenu(MF_STRING, IDC_FILE_ADL_SEARCH, CTSTRING(MENU_ADL_SEARCH));
	//view.AppendMenu(MF_SEPARATOR);
	//view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CTSTRING(MENU_CDMDEBUG_MESSAGES));
        if (advancedMenu) {
	  view.AppendMenu(MF_STRING, IDC_SEARCH_SPY, CTSTRING(MENU_SEARCH_SPY));
          view.AppendMenu(MF_STRING, IDC_NOTEPAD, CTSTRING(MENU_NOTEPAD));
        }
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_FAVORITES, CTSTRING(MENU_FAVORITE_HUBS));
	view.AppendMenu(MF_STRING, IDC_ADVICE_WINDOW, CTSTRING(MENU_ADVICE_WINDOW));
	view.AppendMenu(MF_SEPARATOR);
	//view.AppendMenu(MF_STRING, IDC_TOPMOST, CTSTRING(MENU_TOPMOST));
	//view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_HASH_PROGRESS, CTSTRING(MENU_HASH_PROGRESS));
        if (advancedMenu) {
          view.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
        }
	view.AppendMenu(MF_STRING, ID_VIEW_STATUS_BAR, CTSTRING(MENU_STATUS_BAR));
	view.AppendMenu(MF_STRING, ID_VIEW_TRANSFER_VIEW, CTSTRING(MENU_TRANSFER_VIEW));
        if (advancedMenu) {
          view.AppendMenu(MF_STRING, ID_TOGGLE_TOOLBAR, CTSTRING(TOGGLE_TOOLBAR));
        }
	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)view, CTSTRING(MENU_VIEW));

	CMenuHandle options;
	options.CreatePopupMenu();
        //options.AppendMenu(MF_STRING, IDC_OPTION_DISPLAY_BALOON_POPUPS, CTSTRING(MENU_OPTIONS_DISPLAY_BALOON_POPUPS));
        options.AppendMenu(MF_STRING, IDC_SHUTDOWN, CTSTRING(SHUTDOWN));
        options.AppendMenu(MF_STRING, IDC_THROTTLE_ENABLE, CTSTRING(SETCZDC_ENABLE_LIMITING));
        options.AppendMenu(MF_STRING, IDC_ENABLE_SOUNDS, CTSTRING(MENU_ENABLE_SOUNDS));
        options.AppendMenu(MF_STRING, IDC_AWAY, CTSTRING(MENU_AWAY));
	options.AppendMenu(MF_SEPARATOR);
        options.AppendMenu(MF_STRING, ID_FILE_SETTINGS, CTSTRING(MENU_OPTIONS_SETTINGS));
        mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)options, CTSTRING(MENU_OPTIONS));

#if 0
	CMenuHandle window;
	window.CreatePopupMenu();

	window.AppendMenu(MF_STRING, ID_WINDOW_CASCADE, CTSTRING(MENU_CASCADE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_HORZ, CTSTRING(MENU_HORIZONTAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_VERT, CTSTRING(MENU_VERTICAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_ARRANGE, CTSTRING(MENU_ARRANGE));
	window.AppendMenu(MF_STRING, ID_WINDOW_MINIMIZE_ALL, CTSTRING(MENU_MINIMIZE_ALL));
	window.AppendMenu(MF_STRING, ID_WINDOW_RESTORE_ALL, CTSTRING(MENU_RESTORE_ALL));
	window.AppendMenu(MF_SEPARATOR);
	window.AppendMenu(MF_STRING, IDC_CLOSE_DISCONNECTED, CTSTRING(MENU_CLOSE_DISCONNECTED));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_HUBS, CTSTRING(MENU_CLOSE_ALL_HUBS));
	window.AppendMenu(MF_STRING, IDC_CLOSE_HUBS_BELOW, CTSTRING(MENU_CLOSE_HUBS_BELOW));
	window.AppendMenu(MF_STRING, IDC_CLOSE_HUBS_NO_USR, CTSTRING(MENU_CLOSE_HUBS_EMPTY));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_PM, CTSTRING(MENU_CLOSE_ALL_PM));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_OFFLINE_PM, CTSTRING(MENU_CLOSE_ALL_OFFLINE_PM));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_DIR_LIST, CTSTRING(MENU_CLOSE_ALL_DIR_LIST));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_SEARCH_FRAME, CTSTRING(MENU_CLOSE_ALL_SEARCHFRAME));
	window.AppendMenu(MF_STRING, IDC_RECONNECT_DISCONNECTED, CTSTRING(MENU_RECONNECT_DISCONNECTED));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)window, CTSTRING(MENU_WINDOW));
#endif
#ifdef PPA_INCLUDE_DEAD_CODE
	CMenuHandle sites;
	sites.CreatePopupMenu();

    sites.AppendMenu(MF_STRING, IDC_GUIDE, CTSTRING(MENU_SITES_GUIDE));
	sites.AppendMenu(MF_STRING, IDC_GUIDES, CTSTRING(MENU_SITES_GUIDES));
	sites.AppendMenu(MF_SEPARATOR);
	sites.AppendMenu(MF_STRING, IDC_SITES_TAN, CTSTRING(MENU_SITES_TAN));
	sites.AppendMenu(MF_STRING, IDC_SITES_NXP, CTSTRING(MENU_SITES_NXP));
#endif
	CMenuHandle help;
	help.CreatePopupMenu();

	help.AppendMenu(MF_STRING, IDC_HELP_DONATE, CTSTRING(MENU_HELP)); //[*]PPA, [~] Drakon
//	help.AppendMenu(MF_SEPARATOR);
//	help.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
//	help.AppendMenu(MF_STRING, IDC_HELP_DISCUSS, CTSTRING(MENU_DISCUSS));
	help.AppendMenu(MF_SEPARATOR);
	help.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	
	//tstring JoinTeam = Text::toT(STRING(MENU_JOIN_TEAM)) + _T(SITES_FLYLINK_TRAC); // [+] Drakon
	//help.AppendMenu(MF_STRING, IDC_SITES_FLYLINK_TRAC, JoinTeam.c_str()); // [~] Drakon

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)help, CTSTRING(MENU_HLP)); // [~] Drakon

/** @todo fix this so that the system icon is used for dirs as well (we need
			  to mask it so that incomplete folders appear correct */
#if 0	
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memzero(&fi, sizeof(SHFILEINFO));
		fileImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 16, 16);
		::SHGetFileInfo(_T("."), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		fileImages.AddIcon(ic);
		::DestroyIcon(fi.hIcon);
	} else {
		fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	}
#endif

	fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	dirIconIndex = fileImageCount++;
	dirMaskedIndex = fileImageCount++;

   	    fileImageCount++;

        flagImages.CreateFromImage(IDB_FLAGS, 25, 8, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

        // !SMT!-IP
        flagImageCount = flagImages.GetImageCount();
        CBitmap UserLocations;
        if (UserLocations.m_hBitmap = (HBITMAP)::LoadImage(NULL, Text::toT(Util::getConfigPath() + "CustomLocations.bmp").c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)) {
                flagImages.Add(UserLocations, RGB(77,17,77));
        }

	if(SETTING(USERLIST_IMAGE) == "")
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else
		userImages.CreateFromImage(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE); 
	
	LOGFONT lf, lf2;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
        // SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, Text::fromT(encodeFont(lf))); // !SMT!-F
	decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);
	::GetObject((HFONT)GetStockObject(ANSI_FIXED_FONT), sizeof(lf2), &lf2);
	
	lf2.lfHeight = lf.lfHeight;
	lf2.lfWeight = lf.lfWeight;
	lf2.lfItalic = lf.lfItalic;

	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	font = ::CreateFontIndirect(&lf);
	fontHeight = WinUtil::getTextHeight(mainWnd, font);
	lf.lfWeight = FW_BOLD;
	boldFont = ::CreateFontIndirect(&lf);
	lf.lfHeight *= 5;
	lf.lfHeight /= 6;
	smallBoldFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

	if(BOOLSETTING(URL_HANDLER)) {
		registerDchubHandler();
		registerADChubHandler();
		urlDcADCRegistered = true;
	}
	if(BOOLSETTING(MAGNET_REGISTER)) {
		registerMagnetHandler();
		urlMagnetRegistered = true; 
	}

	DWORD dwMajor = 0, dwMinor = 0;
	if(SUCCEEDED(ATL::AtlGetCommCtrlVersion(&dwMajor, &dwMinor))) {
		comCtlVersion = MAKELONG(dwMinor, dwMajor);
	}
	
	hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, NULL, GetCurrentThreadId());
	
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
    grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_PERIOD, CTSTRING(EXTRA_SLOT_TIMEOUT));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));

        // !SMT!-UI
        userSummaryMenu.CreatePopupMenu();
        banIconOnline = (HICON)::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_BANNED_ONLINE), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
        banIconOffline = (HICON)::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_BANNED_OFF), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);

        // !SMT!-S
        speedMenu.CreatePopupMenu();
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_NONE,  _T("No limit"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_SUPER, _T("Superuser"));
        speedMenu.AppendMenu(MF_SEPARATOR);
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_256K, _T("256 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_128K, _T("128 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_64K, _T("64 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_32K, _T("32 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_24K, _T("24 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_16K, _T("16 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_12K, _T("12 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_08K, _T("8 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_05K, _T("5 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_02K, _T("2 Kb/s"));
        speedMenu.AppendMenu(MF_STRING, IDC_SPEED_BAN, _T("Ban user"));

        // !SMT!-PSW
        privateMenu.CreatePopupMenu();
        privateMenu.AppendMenu(MF_STRING, IDC_PM_NONE,    _T("normal"));
        privateMenu.AppendMenu(MF_STRING, IDC_PM_IGNORED, CTSTRING(IGNORE_PRIVATE));
        privateMenu.AppendMenu(MF_STRING, IDC_PM_FREE,    CTSTRING(FREE_PM_ACCESS));

	initColors();
}

void WinUtil::initColors() {
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);

	CHARFORMAT2 cf;
	memzero(&cf, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(cf);
	cf.dwReserved = 0;
	cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	cf.dwEffects = 0;
	cf.crBackColor = SETTING(BACKGROUND_COLOR);
	cf.crTextColor = SETTING(TEXT_COLOR);

	m_TextStyleTimestamp = cf;
	m_TextStyleTimestamp.crBackColor = SETTING(TEXT_TIMESTAMP_BACK_COLOR);
	m_TextStyleTimestamp.crTextColor = SETTING(TEXT_TIMESTAMP_FORE_COLOR);
	if(SETTING(TEXT_TIMESTAMP_BOLD))
		m_TextStyleTimestamp.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_TIMESTAMP_ITALIC))
		m_TextStyleTimestamp.dwEffects |= CFE_ITALIC;

	m_ChatTextGeneral = cf;
	m_ChatTextGeneral.crBackColor = SETTING(TEXT_GENERAL_BACK_COLOR);
	m_ChatTextGeneral.crTextColor = SETTING(TEXT_GENERAL_FORE_COLOR);
	if(SETTING(TEXT_GENERAL_BOLD))
		m_ChatTextGeneral.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_GENERAL_ITALIC))
		m_ChatTextGeneral.dwEffects |= CFE_ITALIC;

	m_TextStyleBold = m_ChatTextGeneral;
	m_TextStyleBold.dwEffects = CFE_BOLD;
	
	m_TextStyleMyNick = cf;
	m_TextStyleMyNick.crBackColor = SETTING(TEXT_MYNICK_BACK_COLOR);
	m_TextStyleMyNick.crTextColor = SETTING(TEXT_MYNICK_FORE_COLOR);
	if(SETTING(TEXT_MYNICK_BOLD))
		m_TextStyleMyNick.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_MYNICK_ITALIC))
		m_TextStyleMyNick.dwEffects |= CFE_ITALIC;

	m_ChatTextMyOwn = cf;
	m_ChatTextMyOwn.crBackColor = SETTING(TEXT_MYOWN_BACK_COLOR);
	m_ChatTextMyOwn.crTextColor = SETTING(TEXT_MYOWN_FORE_COLOR);
	if(SETTING(TEXT_MYOWN_BOLD))
		m_ChatTextMyOwn.dwEffects       |= CFE_BOLD;
	if(SETTING(TEXT_MYOWN_ITALIC))
		m_ChatTextMyOwn.dwEffects       |= CFE_ITALIC;

	m_ChatTextPrivate = cf;
	m_ChatTextPrivate.crBackColor = SETTING(TEXT_PRIVATE_BACK_COLOR);
	m_ChatTextPrivate.crTextColor = SETTING(TEXT_PRIVATE_FORE_COLOR);
	if(SETTING(TEXT_PRIVATE_BOLD))
		m_ChatTextPrivate.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_PRIVATE_ITALIC))
		m_ChatTextPrivate.dwEffects |= CFE_ITALIC;

	m_ChatTextSystem = cf;
	m_ChatTextSystem.crBackColor = SETTING(TEXT_SYSTEM_BACK_COLOR);
	m_ChatTextSystem.crTextColor = SETTING(TEXT_SYSTEM_FORE_COLOR);
	if(SETTING(TEXT_SYSTEM_BOLD))
		m_ChatTextSystem.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SYSTEM_ITALIC))
		m_ChatTextSystem.dwEffects |= CFE_ITALIC;

	m_ChatTextServer = cf;
	m_ChatTextServer.crBackColor = SETTING(TEXT_SERVER_BACK_COLOR);
	m_ChatTextServer.crTextColor = SETTING(TEXT_SERVER_FORE_COLOR);
	if(SETTING(TEXT_SERVER_BOLD))
		m_ChatTextServer.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SERVER_ITALIC))
		m_ChatTextServer.dwEffects |= CFE_ITALIC;

	m_ChatTextLog = m_ChatTextGeneral;
	m_ChatTextLog.crTextColor = OperaColors::blendColors(SETTING(TEXT_GENERAL_BACK_COLOR), SETTING(TEXT_GENERAL_FORE_COLOR), 0.4);

	m_TextStyleFavUsers = cf;
	m_TextStyleFavUsers.crBackColor = SETTING(TEXT_FAV_BACK_COLOR);
	m_TextStyleFavUsers.crTextColor = SETTING(TEXT_FAV_FORE_COLOR);
	if(SETTING(TEXT_FAV_BOLD))
		m_TextStyleFavUsers.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_FAV_ITALIC))
		m_TextStyleFavUsers.dwEffects |= CFE_ITALIC;

	m_TextStyleOPs = cf;
	m_TextStyleOPs.crBackColor = SETTING(TEXT_OP_BACK_COLOR);
	m_TextStyleOPs.crTextColor = SETTING(TEXT_OP_FORE_COLOR);
	if(SETTING(TEXT_OP_BOLD))
		m_TextStyleOPs.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_OP_ITALIC))
		m_TextStyleOPs.dwEffects |= CFE_ITALIC;

	m_TextStyleURL = cf;
	m_TextStyleURL.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR | CFM_LINK | CFM_UNDERLINE;
	m_TextStyleURL.crBackColor = SETTING(TEXT_URL_BACK_COLOR);
	m_TextStyleURL.crTextColor = SETTING(TEXT_URL_FORE_COLOR);
	m_TextStyleURL.dwEffects = CFE_LINK | CFE_UNDERLINE;
	if(SETTING(TEXT_URL_BOLD))
		m_TextStyleURL.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_URL_ITALIC))
		m_TextStyleURL.dwEffects |= CFE_ITALIC;
}

void WinUtil::uninit() {
	fileImages.Destroy();
	userImages.Destroy();
    flagImages.Destroy();
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(smallBoldFont);
	::DeleteObject(bgBrush);

	mainMenu.DestroyMenu();
	grantMenu.DestroyMenu();
    speedMenu.DestroyMenu(); // !SMT!-S
    privateMenu.DestroyMenu(); // !SMT!-PSW

        // !SMT!-UI
    userSummaryMenu.DestroyMenu();

		::DestroyIcon(banIconOnline);
        ::DestroyIcon(banIconOffline);

	UnhookWindowsHookEx(hook);	

}

static int getScreenPixelsPerInchY() {
  CClientDC dc(0);
  return GetDeviceCaps(dc, LOGPIXELSY);
}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
  ::GetObject((HFONT) GetStockObject(DEFAULT_GUI_FONT), sizeof(dest), &dest);
  StringTokenizer<tstring> st(setting, _T(','));
  const TStringList &sl = st.getTokens();
  if (sl.size() == 4) {
    if (!sl[0].empty()) {
      _tcscpy_s(dest.lfFaceName, sl[0].c_str());
    }
    const int fontSize = Util::toInt(Text::fromT(sl[1]));
    dest.lfHeight = fontSize < 0 ? fontSize : -MulDiv(fontSize, getScreenPixelsPerInchY(), 72);
    dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
    dest.lfItalic = (BYTE)Util::toInt(Text::fromT(sl[3]));
  }	
}

tstring WinUtil::encodeFont(LOGFONT const& font) {
  tstring res(font.lfFaceName);
  res += L',';
  res += Util::toStringW(-MulDiv(font.lfHeight, 72, getScreenPixelsPerInchY()));
  res += L',';
  res += Util::toStringW(font.lfWeight);
  res += L',';
  res += Util::toStringW(font.lfItalic);
  return res;
}

int CALLBACK WinUtil::browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData) {
	switch(uMsg) {
	case BFFM_INITIALIZED: 
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
		break;
	}
	return 0;
}

bool WinUtil::browseDirectory(tstring& target, HWND owner /* = NULL */) {
	TCHAR buf[MAX_PATH];
	BROWSEINFO bi;
	LPMALLOC ma;
	
	memzero(&bi, sizeof(bi));
	
	bi.hwndOwner = owner;
	bi.pszDisplayName = buf;
	bi.lpszTitle = CTSTRING(CHOOSE_FOLDER);
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lParam = (LPARAM)target.c_str();
	bi.lpfn = &browseCallbackProc;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL) {
		SHGetPathFromIDList(pidl, buf);
		target = buf;
		
		if(target.size() > 0 && target[target.size()-1] != _T('\\'))
			target+=_T('\\');
		
		if(SHGetMalloc(&ma) != E_FAIL) {
			ma->Free(pidl);
			ma->Release();
		}
		return true;
	}
	return false;
}

bool WinUtil::browseFile(tstring& target, HWND owner /* = NULL */, bool save /* = true */, const tstring& initialDir /* = Util::emptyString */, const TCHAR* types /* = NULL */, const TCHAR* defExt /* = NULL */) {
	TCHAR buf[MAX_PATH];
	OPENFILENAME ofn = { 0 };       // common dialog box structure
	target = Text::toT(Util::validateFileName(Text::fromT(target)));
	_tcscpy_s(buf, MAX_PATH, target.c_str());
	// Initialize OPENFILENAME
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = owner;
	ofn.lpstrFile = buf;
	ofn.lpstrFilter = types;
	ofn.lpstrDefExt = defExt;
	ofn.nFilterIndex = 1;

	if(!initialDir.empty()) {
		ofn.lpstrInitialDir = initialDir.c_str();
	}
	ofn.nMaxFile = sizeof(buf);
	ofn.Flags = (save ? 0: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
	
	// Display the Open dialog box. 
	if ( (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn) ) ==TRUE) {
		target = ofn.lpstrFile;
		return true;
	}
	return false;
}

void WinUtil::setClipboard(const tstring& str) {
	if(!::OpenClipboard(mainWnd)) {
		return;
	}

	EmptyClipboard();

#ifdef UNICODE	
	OSVERSIONINFOEX ver;
	if( WinUtil::getVersionInfo(ver) ) {
		if( ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
			string tmp = Text::wideToAcp(str);

			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (tmp.size() + 1) * sizeof(char)); 
			if (hglbCopy == NULL) { 
				CloseClipboard(); 
				return; 
			} 

			// Lock the handle and copy the text to the buffer. 
			char* lptstrCopy = (char*)GlobalLock(hglbCopy); 
			strcpy(lptstrCopy, tmp.c_str());
			GlobalUnlock(hglbCopy);

			SetClipboardData(CF_TEXT, hglbCopy);

			CloseClipboard();

			return;
		}
	}
#endif

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR)); 
	if (hglbCopy == NULL) { 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer. 
	TCHAR* lptstrCopy = (TCHAR*)GlobalLock(hglbCopy); 
	_tcscpy(lptstrCopy, str.c_str());
	GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hglbCopy); 
#else
	SetClipboardData(CF_TEXT hglbCopy);
#endif

	CloseClipboard();
}

void WinUtil::splitTokens(int* array, const string& tokens, int maxItems /* = -1 */) throw() {
	StringTokenizer<string> t(tokens, _T(','));
	StringList& l = t.getTokens();
	if(maxItems == -1)
		maxItems = l.size();
	
	int k = 0;
	for(StringList::const_iterator i = l.begin(); i != l.end() && k < maxItems; ++i, ++k) {
		array[k] = Util::toInt(*i);
	}
}

bool WinUtil::getUCParams(HWND parent, const UserCommand& uc, StringMap& sm) throw() {
	string::size_type i = 0;
	StringMap done;

	while( (i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			LineDlg dlg;
			dlg.title = Text::toT(uc.getName());
			dlg.description = Text::toT(name);
			dlg.line = Text::toT(sm["line:" + name]);
			if(dlg.DoModal(parent) == IDOK) {
				sm["line:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	i = 0;
	while( (i = uc.getCommand().find("%[kickline:", i)) != string::npos) {
		i += 11;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			KickDlg dlg;
			dlg.title = Text::toT(uc.getName());
			dlg.description = Text::toT(name);
			if(dlg.DoModal(parent) == IDOK) {
				sm["kickline:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

tstring WinUtil::getChatHelp() {
  tstring CommandsStart = Text::toT(STRING(CMD_FIRST_LINE)) + _T("\n------------------------------------------------------------------------------------------------------------------------------------------------------------" );
  tstring CommandsPart1 = _T("\n/away, /a (message) \t\t\t") + Text::toT(STRING(CMD_AWAY_MSG)) + _T("\n/clear, /c \t\t\t\t") + Text::toT(STRING(CMD_CLEAR_CHAT)) + _T("\n/favshowjoins, /fsj \t\t\t") + Text::toT(STRING(CMD_FAV_JOINS)) + _T("\n/showjoins, /sj \t\t\t\t") + Text::toT(STRING(CMD_SHOW_JOINS)) + _T("\n/ts \t\t\t\t\t") + Text::toT(STRING(CMD_TIME_STAMP)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------");
  tstring CommandsPart2 = _T("\n/slots, /sl # \t\t\t\t") + Text::toT(STRING(CMD_SLOTS)) + _T("\n/extraslots, /es # \t\t\t\t") + Text::toT(STRING(CMD_EXTRA_SLOTS)) + _T("\n/smallfilesize, /sfs # \t\t\t") + Text::toT(STRING(CMD_SMALL_FILES)) + _T("\n/refresh \t\t\t\t") + Text::toT(STRING(CMD_SHARE_REFRESH)) + _T("\n/rebuild \t\t\t\t\t") + Text::toT(STRING(CMD_SHARE_REBUILD)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------");
  tstring CommandsPart3 = _T("\n/join, /j # \t\t\t\t\t") + Text::toT(STRING(CMD_JOIN_HUB)) + _T("\n/close \t\t\t\t\t") + Text::toT(STRING(CMD_CLOSE_WND)) + _T("\n/favorite, /fav \t\t\t\t") + Text::toT(STRING(CMD_FAV_HUB)) + _T("\n/rmfavorite, /rf \t\t\t\t") + Text::toT(STRING(CMD_RM_FAV)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------");
  tstring CommandsPart4 = _T("\n/userlist, /ul \t\t\t\t") + Text::toT(STRING(CMD_USERLIST)) + _T("\n/ignorelist, /il \t\t\t\t") + Text::toT(STRING(CMD_IGNORELIST)) + _T("\n/favuser, /fu # \t\t\t\t") + Text::toT(STRING(CMD_FAV_USER)) + _T("\n/pm (user) (message) \t\t\t") + Text::toT(STRING(CMD_SEND_PM)) + _T("\n/getlist, /gl (user) \t\t\t") + Text::toT(STRING(CMD_GETLIST)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------");
  tstring CommandsPart5 = _T("\n/flylinkdc++ \t\t\t\t") + Text::toT(STRING(CMD_FLYLINKDC)) + _T("\n/connection, /con \t\t\t") + Text::toT(STRING(CMD_CONNECTION)) + _T("\n/stats \t\t\t\t\t") + Text::toT(STRING(CMD_STATS)) + _T("\n/u (url) \t\t\t\t\t") + Text::toT(STRING(CMD_URL)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------");
  tstring CommandsPart6 = _T("\n/savequeue, /sq \t\t\t\t") + Text::toT(STRING(CMD_SAVE_QUEUE)) + _T("\n/search, /s (string) \t\t\t") + Text::toT(STRING(CMD_DO_SEARCH)) + _T("\n/shutdown \t\t\t\t") + Text::toT(STRING(CMD_SHUTDOWN)) + _T("\n/me \t\t\t\t\t") + Text::toT(STRING(CMD_ME)) + _T("\n/winamp, /w (/wmp,/itunes,/mpc) \t\t") + Text::toT(STRING(CMD_WINAMP)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
  tstring CommandsEnd = Text::toT(STRING(CMD_HELP_INFO)) + _T(" \n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
  return CommandsStart + CommandsPart1 + CommandsPart2 + CommandsPart3 + CommandsPart4 + CommandsPart5 + CommandsPart6 + CommandsEnd;
}

bool WinUtil::checkCommand(const tstring& cmd, const tstring& param, tstring& message, tstring& status) {
	if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
		if(Util::stricmp(param.c_str(), _T("system")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + "system.log")));
		} else if(Util::stricmp(param.c_str(), _T("downloads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_DOWNLOAD), time(NULL)))));
		} else if(Util::stricmp(param.c_str(), _T("uploads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_UPLOAD), time(NULL)))));
		} else {
			return false;
		}
	} else if(Util::stricmp(cmd.c_str(), _T("refresh"))==0) {
		try {
			ShareManager::getInstance()->setDirty();
			ShareManager::getInstance()->refresh(true);
		} catch(const ShareException& e) {
			status = Text::toT(e.getError());
		}
	} else if(Util::stricmp(cmd.c_str(), _T("slots"))==0) {
		int j = Util::toInt(Text::fromT(param));
		if(j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, j);
			status = TSTRING(SLOTS_SET);
			ClientManager::getInstance()->infoUpdated(false);
		} else {
			status = TSTRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if(Util::stricmp(cmd.c_str(), _T("search")) == 0) {
		if(!param.empty()) {
			SearchFrameFactory::openWindow(param);
		} else {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		}
	}
/*[-]PPA
else if((Util::stricmp(cmd.c_str(), _T("filext")) == 0) || (Util::stricmp(cmd.c_str(), _T("fx")) == 0)){
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.filext.com/detaillist.php?extdetail=") + Text::toT(Util::encodeURI(Text::fromT(param))) + _T("&Submit3=Go%21"));
		}
	} else if((Util::stricmp(cmd.c_str(), _T("y")) == 0) || (Util::stricmp(cmd.c_str(), _T("yahoo")) == 0)) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://search.yahoo.com/search?p=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
		//Google defination search support
	} else if(Util::stricmp(cmd.c_str(), _T("define")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?hl=en&q=define%3A+") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
		// Lee's /uptime support, why can't he always ask this kind of easy things
	} 
*/
	else if((Util::stricmp(cmd.c_str(), _T("uptime")) == 0) || (Util::stricmp(cmd.c_str(), _T("ut")) == 0)) {
		message = Text::toT("+me uptime: " + WinUtil::formatTime(Util::getUptime()));
		// Lee's /ratio support, why can't he always ask this kind of easy things
	} else if((Util::stricmp(cmd.c_str(), _T("ratio")) == 0) || (Util::stricmp(cmd.c_str(), _T("r")) == 0)) {
		char ratio[128];
		if(SETTING(TOTAL_DOWNLOAD) > 0) {
			_snprintf(ratio, 127, "%.2f", ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));
			ratio[127] = 0;
		}
//[+] WhiteD. Custom ratio message
		StringMap params;
        params["ratio"] = ratio;
        params["up"] = Util::formatBytes(SETTING(TOTAL_UPLOAD));
        params["down"] = Util::formatBytes(SETTING(TOTAL_DOWNLOAD));
        message = Text::toT(Util::formatParams(SETTING(RATIO_TEMPLATE), params, false)).c_str();
		//message = _T("+me ratio: ") + Text::toT(ratio) + Text::toT(" (Uploaded: " + Util::formatBytes(SETTING(TOTAL_UPLOAD)) + " | Downloaded: " + Util::formatBytes(SETTING(TOTAL_DOWNLOAD)) + ")").c_str();
// End of addition.		
		// limiter toggle
	} else if(Util::stricmp(cmd.c_str(), _T("limit")) == 0) {
		if(BOOLSETTING(THROTTLE_ENABLE) == true) {
			Util::setLimiter(false);
			MainFrame::setLimiterButton(false);
			status = TSTRING(LIMITER_OFF);
		} else {
			Util::setLimiter(true);
			MainFrame::setLimiterButton(true);
			status = TSTRING(LIMITER_ON);
		}
                // WMP9+ Support
	} else if(Util::stricmp(cmd.c_str(), _T("wmp")) == 0) {
		string spam = WinUtil::getWMPSpam(FindWindow(_T("WMPlayerApp"), NULL));
		if(!spam.empty()) {
			if(spam != "no_media") {
				message = Text::toT(spam);
			} else {
				status = _T("You have no media playing in Windows Media Player");
			}
		} else {
			status = _T("Supported version of Windows Media Player is not running");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("itunes")) == 0) {
		string spam = WinUtil::getItunesSpam(FindWindow(_T("iTunes"), _T("iTunes")));
		if(!spam.empty()) {
			if(spam != "no_media") {
				message = Text::toT(spam);
			} else {
				status = _T("You have no media playing in iTunes");
			}
		} else {
			status = _T("Supported version of iTunes is not running");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("mpc")) == 0) {
		string spam = WinUtil::getMPCSpam();
		if(!spam.empty()) {
			message = Text::toT(spam);
		} else {
			status = _T("Supported version of Media Player Classic is not running");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("away")) == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			MainFrame::setAwayButton(false);
			status = TSTRING(AWAY_MODE_OFF);
		} else {
			Util::setAway(true);
			MainFrame::setAwayButton(true);
			Util::setAwayMessage(Text::fromT(param));
			status = TSTRING(AWAY_MODE_ON) + Text::toT(Util::getAwayMessage());
		}
		ClientManager::getInstance()->infoUpdated(true);
	} 
/*[-]PPA
else if(Util::stricmp(cmd.c_str(), _T("g")) == 0 || Util::stricmp(cmd.c_str(), _T("google")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("imdb")) == 0 || Util::stricmp(cmd.c_str(), _T("i")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("wikipedia")) == 0 || Util::stricmp(cmd.c_str(), _T("wiki")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://en.wikipedia.org/w/index.php?title=Special%3ASearch&search=") + Text::toT(Util::encodeURI(Text::fromT(param))) + _T("&fulltext=Search"));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("discogs")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.discogs.com/search?type=all&q=") + Text::toT(Util::encodeURI(Text::fromT(param))) + _T("&btn=Search"));
		}
	}
*/
	else if(Util::stricmp(cmd.c_str(), _T("u")) == 0) {
		if (!param.empty()) {
			WinUtil::openLink(Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("rebuild")) == 0) {
		HashManager::getInstance()->rebuild();
	} else if(Util::stricmp(cmd.c_str(), _T("shutdown")) == 0) {
		MainFrame::setShutDown(!(MainFrame::getShutDown()));
		if (MainFrame::getShutDown()) {
			status = TSTRING(SHUTDOWN_ON);
		} else {
			status = TSTRING(SHUTDOWN_OFF);
			}
	} else if((Util::stricmp(cmd.c_str(), _T("winamp")) == 0) || (Util::stricmp(cmd.c_str(), _T("w")) == 0) || (Util::stricmp(cmd.c_str(), _T("f")) == 0) || (Util::stricmp(cmd.c_str(), _T("foobar")) == 0)) {
		string spam = WinUtil::getWinampSpam(FindWindow(_T("Winamp v1.x"), NULL));
		if(!spam.empty()) {
			message = Text::toT(spam);
		} else {
			if((Util::stricmp(cmd.c_str(), _T("f")) == 0) || (Util::stricmp(cmd.c_str(), _T("foobar")) == 0)) {
				status = _T("Supported version of Foobar2000 is not running or component foo_winamp_spam not installed");
			} else {
				status = TSTRING(WINAMP_NOT_RUNNING);
			}
		}
	} else if(Util::stricmp(cmd.c_str(), _T("tvtome")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else
			WinUtil::openLink(_T("http://www.tvtome.com/tvtome/servlet/Search?searchType=all&searchString=") + Text::toT(Util::encodeURI(Text::fromT(param))));
	} else if(Util::stricmp(cmd.c_str(), _T("csfd")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.csfd.cz/search.php?search=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else {
		return false;
	}

	return true;
}

#ifdef PPA_INCLUDE_BITZI_LOOKUP
 void WinUtil::bitziLink(const TTHValue& /*[-]PPA aHash */ ) {
	// to use this free service by bitzi, we must not hammer or request information from bitzi
	// except when the user requests it (a mass lookup isn't acceptable), and (if we ever fetch
	// this data within DC++, we must identify the client/mod in the user agent, so abuse can be 
	// tracked down and the code can be fixed
 	::MessageBox(0,_T("bitziLink (внешний трафик) в FlylinkDC++ отключен!"),0, MB_OK);
	openLink(_T("http://bitzi.com/lookup/tree:tiger:") + Text::toT(aHash.toBase32()));
 }
#endif
 
string WinUtil::getMagnet(const TTHValue& aHash, const tstring& aFile, int64_t aSize) {
        return "magnet:?xt=urn:tree:tiger:" + aHash.toBase32() + "&xl=" + Util::toString(aSize) + "&dn=" + Util::encodeURI(Text::fromT(aFile));
}
string WinUtil::getWebMagnet(const TTHValue& aHash, const tstring& aFile, int64_t aSize) {
        // !necros!
        StringMap params;
        params["magnet"] = getMagnet(aHash, aFile, aSize);
        params["size"] = Util::formatBytes(aSize);
        params["TTH"] = aHash.toBase32();
        params["name"] = Text::fromT(aFile);
        return Util::formatParams(SETTING(COPY_WMLINK), params, false);
}

 void WinUtil::copyMagnet(const TTHValue& aHash, const tstring& aFile, int64_t aSize) {
	if(!aFile.empty()) {
                setClipboard(Text::toT(getMagnet(aHash, aFile, aSize)));
	}
}

 void WinUtil::searchHash(const TTHValue& aHash) {
	SearchFrameFactory::searchTTH(Text::toT(aHash.toBase32()));
 }

 static bool openUserClasses(CRegKey& userClasses) {
	 return userClasses.Open(HKEY_CURRENT_USER, _T("Software\\Classes")) == ERROR_SUCCESS;
 }

 void WinUtil::registerDchubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" /magnet %1");
	Buf[0] = 0;

	CRegKey userClasses;
	if (!openUserClasses(userClasses)) {
		return;
	}
	if(::RegOpenKeyEx(userClasses, _T("dchub\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(Util::stricmp(app.c_str(), Buf) != 0) {
		if (::RegCreateKeyEx(userClasses, _T("dchub"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_DCHUB));
			return;
		}
	
		TCHAR* tmp = _T("URL:Direct Connect Protocol");
		::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		::RegCloseKey(hk);

		::RegCreateKeyEx(userClasses, _T("dchub\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);

		::RegCreateKeyEx(userClasses, _T("dchub\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		app = Text::toT(getAppName());
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);
	}
}

 void WinUtil::unRegisterDchubHandler() {
	CRegKey userClasses;
	if (openUserClasses(userClasses)) {
		SHDeleteKey(userClasses, _T("dchub"));
	}
 }

 void WinUtil::registerADChubHandler() {
	 HKEY hk;
	 TCHAR Buf[512];
	 tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" /magnet %1");
	 Buf[0] = 0;

	 CRegKey userClasses;
	 if (!openUserClasses(userClasses)) {
		 return;
	 }
	 if(::RegOpenKeyEx(userClasses, _T("adc\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		 DWORD bufLen = sizeof(Buf);
		 DWORD type;
		 ::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		 ::RegCloseKey(hk);
	 }

	 if(Util::stricmp(app.c_str(), Buf) != 0) {
		 if (::RegCreateKeyEx(userClasses, _T("adc"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC));
			 return;
		 }

		 TCHAR* tmp = _T("URL:Direct Connect Protocol");
		 ::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		 ::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(userClasses, _T("adc\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(userClasses, _T("adc\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 app = Text::toT(getAppName());
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);
	 }
 }

 void WinUtil::unRegisterADChubHandler() {
	 CRegKey userClasses;
	 if (openUserClasses(userClasses)) {
		 SHDeleteKey(userClasses, _T("adc"));
	 }
 }

void WinUtil::registerMagnetHandler() {
	HKEY hk;
	TCHAR buf[512];
	tstring openCmd, magnetLoc, magnetExe;
	buf[0] = 0;
	bool haveMagnet = false;

	CRegKey userClasses;
	if (!openUserClasses(userClasses)) {
		return;
	}
	// what command is set up to handle magnets right now?
	if(::RegOpenKeyEx(userClasses, _T("magnet\\shell\\open\\command"), 0, KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(TCHAR) * sizeof(buf);
		::RegQueryValueEx(hk, NULL, NULL, NULL, (LPBYTE)buf, &bufLen);
		::RegCloseKey(hk);
	}
	openCmd = buf;
	buf[0] = 0;
#if 0
	// read the location of magnet.exe
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet"), NULL, KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(buf) * sizeof(TCHAR);
		::RegQueryValueEx(hk, _T("Location"), NULL, NULL, (LPBYTE)buf, &bufLen);
		::RegCloseKey(hk);
	}
	magnetLoc = buf;
	string::size_type i;
	if (!magnetLoc.empty() && magnetLoc[0]==_T('"') && string::npos != (i = magnetLoc.find(_T('"'), 1))) {
		magnetExe = magnetLoc.substr(1, i-1);
	}
	// check for the existence of magnet.exe
	if(File::getSize(Text::fromT(magnetExe)) == -1) {
		magnetExe = Text::toT(Util::getDataPath() + "magnet.exe");
		if(File::getSize(Text::fromT(magnetExe)) == -1) {
			// gracefully fall back to registering DC++ to handle magnets
			magnetExe = Text::toT(getAppName());
			haveMagnet = false;
		} else {
			// set Magnet\Location
			if (::RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
				LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_MAGNET));
				return;
			}

			::RegSetValueEx(hk, _T("Location"), NULL, REG_SZ, (LPBYTE)magnetExe.c_str(), sizeof(TCHAR) * (magnetExe.length()+1));
			::RegCloseKey(hk);
		}
	}
#endif
	magnetExe = _T('"') + Text::toT(getAppName()) + _T("\"");
	magnetLoc = magnetExe + _T(" /magnet %1");
	// (re)register the handler if magnet.exe isn't the default, or if DC++ is handling it
	if(BOOLSETTING(MAGNET_REGISTER) && (Util::strnicmp(openCmd, magnetLoc, magnetLoc.size()) != 0 || !haveMagnet)) {
		SHDeleteKey(userClasses, _T("magnet"));
		if (::RegCreateKeyEx(userClasses, _T("magnet"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_MAGNET));
			return;
		}
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_SHELL_DESC), sizeof(TCHAR)*(TSTRING(MAGNET_SHELL_DESC).length()+1));
		::RegSetValueEx(hk, _T("URL Protocol"), NULL, REG_SZ, NULL, NULL);
		::RegCloseKey(hk);
		::RegCreateKeyEx(userClasses, _T("magnet\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)magnetExe.c_str(), sizeof(TCHAR)*(magnetExe.length()+1));
		::RegCloseKey(hk);
		::RegCreateKeyEx(userClasses, _T("magnet\\shell\\open\\command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)magnetLoc.c_str(), sizeof(TCHAR)*(magnetLoc.length()+1));
		::RegCloseKey(hk);
	}
	// magnet-handler specific code
	// clean out the DC++ tree first
	SHDeleteKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet\\Handlers\\DC++"));
	// add DC++ to magnet-handler's list of applications
	::RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet\\Handlers\\DC++"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
	::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_HANDLER_ROOT), sizeof(TCHAR) * (TSTRING(MAGNET_HANDLER_ROOT).size()+1));
	::RegSetValueEx(hk, _T("Description"), NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_HANDLER_DESC), sizeof(TCHAR) * (STRING(MAGNET_HANDLER_DESC).size()+1));
	// set ShellExecute
	tstring app = Text::toT("\"" + getAppName() + "\" /magnet %URL");
	::RegSetValueEx(hk, _T("ShellExecute"), NULL, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length()+1));
	// set DefaultIcon
	app = Text::toT('"' + getAppName() + '"');
	::RegSetValueEx(hk, _T("DefaultIcon"), NULL, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR)*(app.length()+1));
	::RegCloseKey(hk);

	// These two types contain a tth root, and are in common use.  The other two are variations picked up
	// from Shareaza's source, which come second hand from Gordon Mohr.  -GargoyleMT
	// Reference: http://forums.shareaza.com/showthread.php?threadid=23731
	// Note: the three part hash types require magnethandler >= 1.0.0.3
	DWORD nothing = 0;
	::RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Magnet\\Handlers\\DC++\\Type"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
	::RegSetValueEx(hk, _T("urn:bitprint"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("urn:tigertree"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));	// used by nextpeer :(
	::RegSetValueEx(hk, _T("urn:tree:tiger"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("urn:tree:tiger/"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("urn:tree:tiger/1024"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	// Short URN versions
	::RegSetValueEx(hk, _T("bitprint"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("tigertree"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));	// used by nextpeer :(
	::RegSetValueEx(hk, _T("tree:tiger"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("tree:tiger/"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegSetValueEx(hk, _T("tree:tiger/1024"), NULL, REG_DWORD, (LPBYTE)&nothing, sizeof(nothing));
	::RegCloseKey(hk);
}

void WinUtil::unRegisterMagnetHandler() {
	SHDeleteKey(HKEY_CLASSES_ROOT, _T("magnet"));
	SHDeleteKey(HKEY_LOCAL_MACHINE, _T("magnet"));
}

void WinUtil::allowUserTOSSetting() {
	HKEY hk;
	DWORD value = 0;
	// Allow setting TOS in Windows <= XP
	// see http://support.microsoft.com/kb/248611
	if (::RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
		::RegSetValueEx(hk, _T("DisableUserTOSSetting"), NULL, REG_DWORD, (LPBYTE)&value, sizeof(value));
		::RegCloseKey(hk);
	}
}

bool WinUtil::handleLink(const tstring& url) {
	if (_tcsnicmp(url.c_str(), _T("magnet:?"), 8) == 0) {
		WinUtil::parseMagnetUri(url);
		return true;
	}
	if (_tcsnicmp (url.c_str(), _T("dchub://"), 8) == 0) {
		WinUtil::parseDchubUrl(url);
		return true;
	}
	return false;
}

void WinUtil::openLink(const tstring& url) {
	CRegKey key;
	TCHAR regbuf[MAX_PATH];
	ULONG len = MAX_PATH;
	if(_strnicmp(Text::fromT(url).c_str(), "magnet:?", 8) == 0) {
		parseMagnetUri(url);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "dchub://", 8) == 0) {
		parseDchubUrl(url);
		return;
	}
	tstring x;

	tstring::size_type i = url.find(_T("://"));
	if(i != string::npos) {
		x = url.substr(0, i);
	} else {
		x = _T("http");
	}
	x += _T("\\shell\\open\\command");
	if(key.Open(HKEY_CLASSES_ROOT, x.c_str(), KEY_READ) == ERROR_SUCCESS) {
		if(key.QueryStringValue(NULL, regbuf, &len) == ERROR_SUCCESS) {
			/*
			 * Various values (for http handlers):
			 *  C:\PROGRA~1\MOZILL~1\FIREFOX.EXE -url "%1"
			 *  "C:\Program Files\Internet Explorer\iexplore.exe" -nohome
			 *  "C:\Apps\Opera7\opera.exe"
			 *  C:\PROGRAMY\MOZILLA\MOZILLA.EXE -url "%1"
			 *  C:\PROGRA~1\NETSCAPE\NETSCAPE\NETSCP.EXE -url "%1"
			 */
			tstring cmd(regbuf); // otherwise you consistently get two trailing nulls
			
			if(cmd.length() > 1) {
				string::size_type start,end;
				if(cmd[0] == '"') {
					start = 1;
					end = cmd.find('"', 1);
				} else {
					start = 0;
					end = cmd.find(' ', 1);
				}
				if(end == string::npos)
					end = cmd.length();

				tstring cmdLine(cmd);
				cmd = cmd.substr(start, end-start);
				size_t arg_pos;
				if((arg_pos = cmdLine.find(_T("%1"))) != string::npos) {
					cmdLine.replace(arg_pos, 2, url);
				} else {
					cmdLine.append(_T(" \"") + url + _T('\"'));
				}

				STARTUPINFO si = { sizeof(si), 0 };
				PROCESS_INFORMATION pi = { 0 };
				const int iLen = cmdLine.length() + 1;
				AutoArray<TCHAR> buf(iLen);
				_tcscpy_s(buf, iLen, cmdLine.c_str());
				if(::CreateProcess(cmd.c_str(), buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					::CloseHandle(pi.hThread);
					::CloseHandle(pi.hProcess);
					return;
				}
			}
		}
	}

	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::parseDchubUrl(const tstring& aUrl) {
	string server, file;
	uint16_t port = 411;
	Util::decodeUrl(Text::fromT(aUrl), server, port, file);
	if(!server.empty()) {
		HubFrameFactory::openWindow(Text::toT(server) + _T(":") + Util::toStringW(port),false);
	}
	if(!file.empty()) {
		if(file[0] == '/') // Remove any '/' in from of the file
			file = file.substr(1);
		try {
			if(!file.empty()) {
				UserPtr user = ClientManager::getInstance()->findLegacyUser(file);
				if(user)
					QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
			}
			// @todo else report error
		} catch(const Exception&) {
			// ...
		}
	}
}

void WinUtil::parseADChubUrl(const tstring& aUrl) {
	string server, file;
	uint16_t port = 0; //make sure we get a port since adc doesn't have a standard one
	Util::decodeUrl(Text::fromT(aUrl), server, port, file);
	if(!server.empty() && port > 0) {
		HubFrameFactory::openWindow(_T("adc://") + Text::toT(server) + _T(":") + Util::toStringW(port),false);
	}
}

void WinUtil::parseMagnetUri(const tstring& aUrl, bool /*aOverride*/) {
	// official types that are of interest to us
	//  xt = exact topic
	//  xs = exact substitute
	//  as = acceptable substitute
	//  dn = display name
	//  xl = exact length
	if (Util::strnicmp(aUrl.c_str(), _T("magnet:?"), 8) == 0) {
		LogManager::getInstance()->message(STRING(MAGNET_DLG_TITLE) + ": " + Text::fromT(aUrl));
		StringTokenizer<tstring> mag(aUrl.substr(8), _T('&'));
		typedef LokiMap<tstring, tstring> MagMap; 
		MagMap hashes;
		tstring fname, fhash, type, param;
		int64_t fsize = 0;
		bool onlineVideo = false;
		for(TStringList::const_iterator idx = mag.getTokens().begin(); idx != mag.getTokens().end(); ++idx) {
			// break into pairs
			string::size_type pos = idx->find(_T('='));
			if(pos != string::npos) {
				type = Text::toT(Text::toLower(Util::encodeURI(Text::fromT(idx->substr(0, pos)), true)));
				param = Text::toT(Util::encodeURI(Text::fromT(idx->substr(pos+1)), true));
			} else {
				type = Text::toT(Util::encodeURI(Text::fromT(*idx), true));
				param.clear();
			}
			// extract what is of value
			if(param.length() == 85 && Util::strnicmp(param.c_str(), _T("urn:bitprint:"), 13) == 0) {
				hashes[type] = param.substr(46);
			} else if(param.length() == 54 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger:"), 15) == 0) {
				hashes[type] = param.substr(15);
			} else if(param.length() == 53 && Util::strnicmp(param.c_str(), _T("urn:tigertree:"), 14) == 0) {  // used by nextpeer :(
				hashes[type] = param.substr(14);
			}  else if(param.length() == 55 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger/:"), 16) == 0) {
				hashes[type] = param.substr(16);
			} else if(param.length() == 59 && Util::strnicmp(param.c_str(), _T("urn:tree:tiger/1024:"), 20) == 0) {
				hashes[type] = param.substr(20);
			}
			// Short URN versions
			else if(param.length() == 81 && Util::strnicmp(param.c_str(), _T("bitprint:"), 9) == 0) {
				hashes[type] = param.substr(42);
			} else if(param.length() == 50 && Util::strnicmp(param.c_str(), _T("tree:tiger:"), 11) == 0) {
				hashes[type] = param.substr(11);
			} else if(param.length() == 49 && Util::strnicmp(param.c_str(), _T("tigertree:"), 10) == 0) {  // used by nextpeer :(
				hashes[type] = param.substr(10);
			} else if(param.length() == 51 && Util::strnicmp(param.c_str(), _T("tree:tiger/:"), 12) == 0) {
				hashes[type] = param.substr(12);
			} else if(param.length() == 55 && Util::strnicmp(param.c_str(), _T("tree:tiger/1024:"), 16) == 0) {
				hashes[type] = param.substr(16);
			} 
			// File name and size
			else if(type.length() == 2 && Util::strnicmp(type.c_str(), _T("dn"), 2) == 0) {
				fname = param;
			} else if(type.length() == 2 && Util::strnicmp(type.c_str(), _T("xl"), 2) == 0) {
				fsize = _tstoi64(param.c_str());
			} else if(type.length() == 5 && Util::strnicmp(type.c_str(), _T("video"), 5) == 0) {
				onlineVideo = true;
			}
		}
#ifdef _DEBUG
		if (!onlineVideo && Util::stricmp(fname, _T(".avi")) > 0) {
			onlineVideo = true;
		}
#endif
		// pick the most authoritative hash out of all of them.
		if(hashes.find(_T("as")) != hashes.end()) {
			fhash = hashes[_T("as")];
		}
		if(hashes.find(_T("xs")) != hashes.end()) {
			fhash = hashes[_T("xs")];
		}
		if(hashes.find(_T("xt")) != hashes.end()) {
			fhash = hashes[_T("xt")];
		}
		if(!fhash.empty() && Encoder::isBase32(Text::fromT(fhash).c_str())){
			if (onlineVideo) {
				if (!fname.empty()) {
					// TODO special location
					string path = SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(fname);
					if (File::exists(path)) {
						showVideo(path);
						return;
					}
				}
				TTHValue tmphash(Text::fromT(fhash));
				string localPath = ShareManager::getInstance()->toRealPath(tmphash);
				if (!localPath.empty()) {
					showVideo(localPath);
					return;
				}
				QueueItem::List ql;
				QueueManager::getInstance()->getTargets(tmphash, ql);
				if (!ql.empty()) {
					ql.front()->setFlag(QueueItem::FLAG_ONLINE_VIDEO);
					showVideoSplash();
					return;
				}
				showVideoSplash();
			}
			// ok, we have a hash, and maybe a filename.
			bool magnetDirectHandle = fsize > 0 && !fname.empty();
#ifdef MAGNET_DIALOG
			if (magnetDirectHandle) { magnetDirectHandle = !BOOLSETTING(MAGNET_ASK); }
#endif
			if (onlineVideo && fsize > 0 && !fname.empty()) {
				// TODO download to special location
				QueueManager::getInstance()->add(Text::fromT(fname), fsize, Text::fromT(fhash), QueueItem::FLAG_ONLINE_VIDEO|QueueItem::FLAG_MULTI_SOURCE);
			}
			else if (magnetDirectHandle) {
				int magnetAction = SettingsManager::MAGNET_AUTO_DOWNLOAD;
				switch(magnetAction)
				{
				case SettingsManager::MAGNET_AUTO_DOWNLOAD:
					QueueManager::getInstance()->add(Text::fromT(fname), fsize, Text::fromT(fhash));
					break;
				case SettingsManager::MAGNET_AUTO_SEARCH:
					SearchFrameFactory::searchTTH(fhash);
					break;
				}
			} else {
				// use aOverride to force the display of the dialog.  used for auto-updating
				MagnetDlg dlg(fhash, fname, fsize);
				dlg.DoModal(mainWnd);
			}
		} 
		else {
			MessageBox(mainWnd, CTSTRING(MAGNET_DLG_TEXT_BAD), CTSTRING(MAGNET_DLG_TITLE), MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

class PlayerLauncherThread : private Thread {
private:
	void playVideo(const string& path) {
		LogManager::getInstance()->message(Text::fromT(_T("Starting video player " + Text::toT(path))));
		string application;
		string file;
		if (!BOOLSETTING(USE_CUSTOM_VIDEO_PLAYER)) {
			application = Util::getFilePath(WinUtil::getAppName()) + "\\VLC\\VLCPortable.exe";
			file = "--play-and-stop \"" + path + "\"";
		}
		else {
			application = SETTING(CUSTOM_VIDEO_PLAYER);
			file = "\"" + path + "\"";
		}
		string dir = "\"" + Util::getFilePath(path) + "\"";
		::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(file).c_str(), Text::toT(dir).c_str(), SW_SHOWNORMAL);
	}

	void playSplash() {
		if (BOOLSETTING(USE_CUSTOM_VIDEO_PLAYER)) return;
		LogManager::getInstance()->message(Text::fromT(_T("Starting video player")));
		string vlcPath = Util::getFilePath(WinUtil::getAppName()) + "\\VLC\\";
		string application = vlcPath + "VLCPortable.exe";
		// --qt-display-mode=2
		string commandLine = "--repeat --no-video-title-show \"" + vlcPath + "downloading.wmv\"";
		string dir = "\"" + vlcPath + "\"";
		::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(commandLine).c_str(), Text::toT(dir).c_str(), SW_SHOWNORMAL);
	}

	virtual int run() {
		int count = 0;
		for (;;) {
			string path;
			{
				Lock lock(m_cs);
				if (!m_paths.empty()) {
					path = m_paths.front();
					m_paths.pop_front();
				}
			}
			if (path.empty()) {
				if (++count > 10) {
					Lock lock(m_cs);
					if (m_paths.empty()) {
						m_running = false;
						return 0;
					}
				}
				Thread::sleep(100);
				continue;
			}
			count = 0;
#ifdef _DEBUG
			TracePrint("play %s", path.c_str());
#endif
			if (path == SPLASH) {
				playSplash();
			}
			else if (path == QUIT) {
				Lock lock(m_cs);
				m_running = false;
				return 0;
			}
			else {
				playVideo(path);
			}
		}
	}

	CriticalSection m_cs;
	deque<string> m_paths;
	bool m_running;
	bool m_shutdown;

	BOOL isRunning() {
		static const TCHAR MUTEX[]  = _T("VLCPortable");
		SingleInstance playerInstance(MUTEX);
		return playerInstance.IsAnotherInstanceRunning();
	}

public:
	static const string SPLASH;
	static const string QUIT;

	PlayerLauncherThread(): m_running(false), m_shutdown(false) { }

	void play(const string& path) {
		Lock lock(m_cs);
		if (m_shutdown) return;
		if (path == SPLASH && isRunning()) return;
		if (find(m_paths.begin(), m_paths.end(), path) == m_paths.end()) {
			m_paths.push_back(path);
			if (!m_running) {
				m_running = true;
				start();
			}
		}
	}

	void stop() {
		{
			Lock lock(m_cs);
			m_shutdown = true;
			if (!m_running) {
				return;
			}
			m_paths.clear();
			m_paths.push_back(QUIT);
		}
		join();
	}

};

const string PlayerLauncherThread::SPLASH = "SPLASH";
const string PlayerLauncherThread::QUIT = "QUIT";

static PlayerLauncherThread& getPlayerLauncher() {
	static PlayerLauncherThread launcher;
	return launcher;
}

void WinUtil::showVideo(const string& path) {	
	getPlayerLauncher().play(path);
}

void WinUtil::showVideoSplash() {
	showVideo(PlayerLauncherThread::SPLASH);
}

void WinUtil::stopVideo() {
	getPlayerLauncher().stop();
}

bool WinUtil::isVideo(const string &fileName) {
	return ShareManager::checkType(fileName, SearchManager::TYPE_VIDEO);
#if 0
	tstring extension = Text::toLower(Text::toT(Util::getFileExt(fileName)));
	if (extension.size() > 1) extension = extension.substr(1); // remove starting dot
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();
	if (lst.size() > 0) {
		for (PreviewApplication::Iter i = lst.begin(); i != lst.end(); ++i) {
		  tstring ext = Text::toLower(Text::toT((*i)->getExtension()));
		  if (ext.find(_T("avi")) == tstring::npos) continue;
		  TStringList tok = StringTokenizer<tstring>(ext, _T(';')).getTokens();
			for (TStringIter si = tok.begin(); si != tok.end(); ++si) {
				if (extension == *si) {
					return true;
				}
			}
		}
	}
	return false;
#endif
}

int WinUtil::textUnderCursor(POINT p, CEdit& ctrl, tstring& x) {
	
	int i = ctrl.CharFromPos(p);
	int line = ctrl.LineFromChar(i);
	int c = LOWORD(i) - ctrl.LineIndex(line);
	int len = ctrl.LineLength(i) + 1;
	if(len < 3) {
		return 0;
	}

	AutoArray<TCHAR> buf(len);
	ctrl.GetLine(line, buf, len);
	x = tstring(buf, len-1);

	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if(start == string::npos)
		start = 0;
	else
		start++;

	return start;
}

bool WinUtil::parseDBLClick(const tstring& aString, string::size_type start, string::size_type end) {
	if( (Util::strnicmp(aString.c_str() + start, _T("http://"), 7) == 0) || 
		(Util::strnicmp(aString.c_str() + start, _T("www."), 4) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("ftp://"), 6) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("irc://"), 6) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("https://"), 8) == 0) ||	
		(Util::strnicmp(aString.c_str() + start, _T("file://"), 7) == 0) ||
		(Util::strnicmp(aString.c_str() + start, _T("mailto:"), 7) == 0) )
	{

		openLink(aString.substr(start, end-start));
		return true;
	} else if(Util::strnicmp(aString.c_str() + start, _T("dchub://"), 8) == 0) {
		parseDchubUrl(aString.substr(start, end-start));
		return true;
	} else if(Util::strnicmp(aString.c_str() + start, _T("magnet:?"), 8) == 0) {
		parseMagnetUri(aString.substr(start, end-start));
		return true;
	} else if(Util::strnicmp(aString.c_str() + start, _T("adc://"), 6) == 0) {
		parseADChubUrl(aString.substr(start, end-start));
		return true;
	}
	return false;
}

// !SMT!-UI (todo: disable - this routine does not save column visibility)
void WinUtil::saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
							  SettingsManager::StrSetting widths, int n, 
                                                          int* indexes, int* sizes) throw()
{
	string tmp;

	ctrl.GetColumnOrderArray(n, indexes);
	int i;
	for(i = 0; i < n; ++i) {
		tmp += Util::toString(indexes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(order, tmp);
	tmp.clear();
	int nHeaderItemsCount = ctrl.GetHeader().GetItemCount();
	for(i = 0; i < n; ++i) {
		sizes[i] = ctrl.GetColumnWidth(i);
		if (i >= nHeaderItemsCount) // Not exist column
			sizes[i] = 0;
		tmp += Util::toString(sizes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(widths, tmp);
}

int WinUtil::getIconIndex(const tstring& aFileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
        memzero(&fi, sizeof(SHFILEINFO)); //[+] CZDC++
		tstring x = Text::toLower(Util::getFileExt(aFileName));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		tstring fn = Text::toLower(Util::getFileName(aFileName));
		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);

		fileIndexes[x] = fileImageCount++;
		return fileImageCount - 1;
	} else {
		return 2;
	}
}

double WinUtil::toBytes(TCHAR* aSize) {
	double bytes = _tstof(aSize);

	if (_tcsstr(aSize, CTSTRING(PB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(TB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(GB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(MB))) {
		return bytes * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(KB))) {
		return bytes * 1024.0;
	} else {
		return bytes;
	}
}

int WinUtil::getOsMajor() {
	OSVERSIONINFOEX ver;
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.dwMajorVersion;
}

int WinUtil::getOsMinor() 
{
	OSVERSIONINFOEX ver;
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.dwMinorVersion;
}
/*
tstring WinUtil::getNicks(const CID& cid) throw() {
	return Util::toStringW(ClientManager::getInstance()->getNicks(cid));
}
*/
pair<tstring, bool> WinUtil::getHubNames(const CID& cid) throw() {
	StringList hubs = ClientManager::getInstance()->getHubNames(cid);
	if(hubs.empty()) {
		return make_pair(TSTRING(OFFLINE), false);
	} else {
		return make_pair(Text::toT(Util::toString(hubs)), true);
	}
}

void WinUtil::getContextMenuPos(CListViewCtrl& aList, POINT& aPt) {
	int pos = aList.GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED);
	if(pos >= 0) {
		CRect lrc;
		aList.GetItemRect(pos, &lrc, LVIR_LABEL);
		aPt.x = lrc.left;
		aPt.y = lrc.top + (lrc.Height() / 2);
	} else {
		aPt.x = aPt.y = 0;
	}
	aList.ClientToScreen(&aPt);
}

void WinUtil::getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt) {
	CRect trc;
	HTREEITEM ht = aTree.GetSelectedItem();
	if(ht) {
		aTree.GetItemRect(ht, &trc, TRUE);
		aPt.x = trc.left;
		aPt.y = trc.top + (trc.Height() / 2);
	} else {
		aPt.x = aPt.y = 0;
	}
	aTree.ClientToScreen(&aPt);
}
void WinUtil::getContextMenuPos(CEdit& aEdit, POINT& aPt) {
	CRect erc;
	aEdit.GetRect(&erc);
	aPt.x = erc.Width() / 2;
	aPt.y = erc.Height() / 2;
	aEdit.ClientToScreen(&aPt);
}

void WinUtil::openFolder(const tstring& file) {
	if (File::getSize(Text::fromT(file)) != -1)
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, /select, \"" + (Text::fromT(file)) + "\"").c_str(), NULL, SW_SHOWNORMAL);
	else
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, \"" + Util::getFilePath(Text::fromT(file)) + "\"").c_str(), NULL, SW_SHOWNORMAL);
}

void WinUtil::ClearPreviewMenu(OMenu &previewMenu){
	while(previewMenu.GetMenuItemCount() > 0) {
		previewMenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

int WinUtil::SetupPreviewMenu(CMenu &previewMenu, string extension){
	int PreviewAppsSize = 0;
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();
	if(lst.size()>0){		
		PreviewAppsSize = 0;
		for(PreviewApplication::Iter i = lst.begin(); i != lst.end(); ++i){
			StringList tok = StringTokenizer<string>((*i)->getExtension(), ';').getTokens();
			bool add = false;
			if(tok.size()==0)add = true;

			
			for(StringIter si = tok.begin(); si != tok.end(); ++si) {
				if(_stricmp(extension.c_str(), si->c_str())==0){
					add = true;
					break;
				}
			}
							
			if (add) previewMenu.AppendMenu(MF_STRING, IDC_PREVIEW_APP + PreviewAppsSize, Text::toT(((*i)->getName())).c_str());
			PreviewAppsSize++;
		}
	}
	return PreviewAppsSize;
}

void WinUtil::RunPreviewCommand(unsigned int index, string target){
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();

	if(index <= lst.size()) {
		string application = lst[index]->getApplication();
		string arguments = lst[index]->getArguments();
		if (application[0] == '.' && application[1] == '\\') {
			application = Util::getFilePath(getAppName()) + application.substr(2);
		}
		StringMap ucParams;				
	
		ucParams["file"] = "\"" + target + "\"";
		ucParams["dir"] = "\"" + Util::getFilePath(target) + "\"";

		::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(Util::formatParams(arguments, ucParams, false)).c_str(), Text::toT(ucParams["dir"]).c_str(), SW_SHOWNORMAL);
	}
}

string WinUtil::formatTime(long rest) {
	char buf[128];
	string formatedTime;
	long n, i;
	i = 0;
	n = rest / (24*3600*7);
	rest %= (24*3600*7);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%d weeks ", n);
		else
			snprintf(buf, sizeof(buf), "%d week ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (24*3600);
	rest %= (24*3600);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%d days ", n); 
		else
			snprintf(buf, sizeof(buf), "%d day ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (3600);
	rest %= (3600);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%d hours ", n);
		else
			snprintf(buf, sizeof(buf), "%d hour ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (60);
	rest %= (60);
	if(n) {
		snprintf(buf, sizeof(buf), "%d min ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest;
	if(++i <= 3) {
		snprintf(buf, sizeof(buf),"%d sec ", n); 
		formatedTime += (string)buf;
	}
	return formatedTime;
}

uint8_t WinUtil::getImage(const Identity& u) {
	uint8_t image = 12;

	if(u.isOp()) {
		image = 0;
	} else if(u.getUser()->isSet(User::FIREBALL)) {
		image = 1;
	} else if(u.getUser()->isSet(User::SERVER)) {
		image = 2;
	} else 
	   {
		string conn = u.getConnection();
/*[-]PPA	
		if(	(conn == "28.8Kbps") ||
			(conn == "33.6Kbps") ||
			(conn == "56Kbps") ||
			(conn == "Modem") ||
			(conn == "ISDN")) {
			image = 6;
		} else
		if(	(conn == "Satellite") ||
			(conn == "Microwave") ||
			(conn == "Wireless")) {
			image = 8;
		} else
		if(	(conn == "DSL") ||
			(conn == "Cable")) {
			image = 9;
		} else
		if(	(strncmp(conn.c_str(), "LAN", 3) == 0)) {
			image = 11;
		} else
#ifdef PPA_INCLUDE_NETLIMITER
		if( (strncmp(conn.c_str(), "NetLimiter", 10) == 0)) {
			image = 3;
		} else
#endif
		if( (conn == "0.005")) {
			image = 5;
		} 
		else 
*/
		{
			double us = Util::toDouble(conn);
			if(us >= 10) {
				image = 10;
			} else 
			if(us > 0.1) {
				image = 7;
			} else
			if(us >= 0.01) {
				image = 4;
			}
		}
	}
	if(u.getUser()->isSet(User::AWAY)) {
		image+=13;
	}
	if(u.getUser()->isSet(User::DCPLUSPLUS)) {
		image+=26;
	}

	if(!u.isTcpActive()) {
		// Users we can't connect to...
		image+=52;
	}		
	return image;
}

int WinUtil::getFlagImage(string& country, bool
#ifdef PPA_USE_COUNTRY
fullname
#endif
) {
        // !SMT!-IP
        if (country.empty()) return 0;
        if (isdigit(country[0])) {
                string::size_type comma = country.find(',');
                if (comma != string::npos) {
                        uint32_t res = flagImageCount + atoi(country.c_str());
                        country = country.substr(comma+1);
                        return res;
                }
        }
#ifdef PPA_USE_COUNTRY
        if(fullname) {
                for(int i = 1; i <= COUNTOF(CountryNames); i++) {
                        if(_stricmp(country.c_str(), CountryNames[i-1]) == 0) {
                                return i;
                        }
                }
        } else 
		{
                for(int i = 1; i <= COUNTOF(CountryCodes); i++) {
                        if(_stricmp(country.c_str(),CountryCodes[i-1]) == 0) {
                                return i;
                        }
                }
        }
#endif
        return 0;
}

float ProcSpeedCalc() {
#define RdTSC __asm _emit 0x0f __asm _emit 0x31
__int64 cyclesStart = 0, cyclesStop = 0;
unsigned __int64 nCtr = 0, nFreq = 0, nCtrStop = 0;
    if(!QueryPerformanceFrequency((LARGE_INTEGER *) &nFreq)) return 0;
    QueryPerformanceCounter((LARGE_INTEGER *) &nCtrStop);
    nCtrStop += nFreq;
    _asm {
		RdTSC
        mov DWORD PTR cyclesStart, eax
        mov DWORD PTR [cyclesStart + 4], edx
    } do {
		QueryPerformanceCounter((LARGE_INTEGER *) &nCtr);
    } while (nCtr < nCtrStop);
    _asm {
		RdTSC
        mov DWORD PTR cyclesStop, eax
        mov DWORD PTR [cyclesStop + 4], edx
    }
	return ((float)cyclesStop-(float)cyclesStart) / 1000000;
}
/*
[-]PPA
int arrayutf[96] = {-61, -127, -60, -116, -60, -114, -61, -119, -60, -102, -61, -115, -60, -67, -59, -121, -61, -109, -59, -104, -59, -96, -59, -92, -61, -102, -59, -82, -61, -99, -59, -67, -61, -95, -60, -115, -60, -113, -61, -87, -60, -101, -61, -83, -60, -66, -59, -120, -61, -77, -59, -103, -59, -95, -59, -91, -61, -70, -59, -81, -61, -67, -59, -66, -61, -124, -61, -117, -61, -106, -61, -100, -61, -92, -61, -85, -61, -74, -61, -68, -61, -76, -61, -108, -60, -71, -60, -70, -60, -67, -60, -66, -59, -108, -59, -107};
int arraywin[48] = {65, 67, 68, 69, 69, 73, 76, 78, 79, 82, 83, 84, 85, 85, 89, 90, 97, 99, 100, 101, 101, 105, 108, 110, 111, 114, 115, 116, 117, 117, 121, 122, 65, 69, 79, 85, 97, 101, 111, 117, 111, 111, 76, 108, 76, 108, 82, 114};

string WinUtil::disableCzChars(string message) {
	string s;

	for(unsigned int j = 0; j < message.length(); j++) {
		int zn = (int)message[j];
		int zzz = -1;
		for(int l = 0; l + 1 < 96; l+=2) {
			if (zn == arrayutf[l]) {
				int zn2 = (int)message[j+1];
				if(zn2 == arrayutf[l+1]) {
					zzz = (int)(l/2);
					break;
				}
			}
		}
		if (zzz >= 0) {
			s += (char)(arraywin[zzz]);
			j++;
		} else {
			s += message[j];
		}
	}

	return s;
}
*/
string WinUtil::generateStats() {
	if(LOBYTE(LOWORD(GetVersion())) >= 5) {
		char buf[2048];
		PROCESS_MEMORY_COUNTERS pmc;
		pmc.cb = sizeof(pmc);
		typedef bool (CALLBACK* LPFUNC)(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
		LPFUNC _GetProcessMemoryInfo = (LPFUNC)GetProcAddress(LoadLibrary(_T("psapi")), "GetProcessMemoryInfo");
		_GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
		GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
		int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
		int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);  
		sprintf_s(buf, sizeof(buf), "\n-=[ " APPNAME " %s Compiled on: %s  ]=-\r\n-=[ Uptime: %s][ Cpu time: %s ]=-\r\n-=[ Memory usage (peak): %s (%s) ]=-\r\n-=[ Virtual memory usage (peak): %s (%s) ]=-\r\n-=[ Downloaded: %s ][ Uploaded: %s ]=-\r\n-=[ Total download: %s ][ Total upload: %s ]=-\r\n-=[ System Uptime: %s]=-\r\n-=[ CPU Clock: %.0f MHz ]=-", 
			VERSIONSTRING,Util::getCompileDate().c_str(), formatTime(Util::getUptime()).c_str(), Text::fromT(Util::formatSeconds((kernelTime + userTime) / (10I64 * 1000I64 * 1000I64))).c_str(), 
			Util::formatBytes(pmc.WorkingSetSize).c_str(), Util::formatBytes(pmc.PeakWorkingSetSize).c_str(), 
			Util::formatBytes(pmc.PagefileUsage).c_str(), Util::formatBytes(pmc.PeakPagefileUsage).c_str(), 
			Util::formatBytes(Socket::getTotalDown()).c_str(), Util::formatBytes(Socket::getTotalUp()).c_str(), 
			Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str(), Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str(), 
			formatTime(GET_TICK()/1000).c_str(), ProcSpeedCalc());
		return buf;
	} else {
		char buf[512];
		snprintf(buf, sizeof(buf), "\r\n-=[ " APPNAME " %s Compiled on: %s ]=-\r\n-=[ Uptime: %s]=-\r\n-=[ Downloaded: %s ][ Uploaded: %s ]=-\r\n-=[ Total download: %s ][ Total upload: %s ]=-", 
			VERSIONSTRING, Util::getCompileDate().c_str(), formatTime(Util::getUptime()).c_str(), 
			Util::formatBytes(Socket::getTotalDown()).c_str(), Util::formatBytes(Socket::getTotalUp()).c_str(), 
			Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str(), Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str());
		return buf;
	}
} 

bool WinUtil::shutDown(int action) {
	// Prepare for shutdown
	UINT iForceIfHung = 0;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi) != 0 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		iForceIfHung = 0x00000010;
		HANDLE hToken;
		OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken);

		LUID luid;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid);
		
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges[0].Luid = luid;
		AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
		CloseHandle(hToken);
	}

	// Shutdown
	switch(action) {
		case 0: { action = EWX_POWEROFF; break; }
		case 1: { action = EWX_LOGOFF; break; }
		case 2: { action = EWX_REBOOT; break; }
		case 3: { SetSuspendState(false, false, false); return true; }
		case 4: { SetSuspendState(true, false, false); return true; }
		case 5: { 
			if(LOBYTE(LOWORD(GetVersion())) >= 5) {
				typedef bool (CALLBACK* LPLockWorkStation)(void);
				LPLockWorkStation _d_LockWorkStation = (LPLockWorkStation)GetProcAddress(LoadLibrary(_T("user32")), "LockWorkStation");
				_d_LockWorkStation();
			}
			return true;
		}
	}

	if (ExitWindowsEx(action | iForceIfHung, 0) == 0) {
		return false;
	} else {
		return true;
	}
}

int WinUtil::getFirstSelectedIndex(CListViewCtrl& list) {
	for(int i = 0; i < list.GetItemCount(); ++i) {
		if (list.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			return i;
		}
	}
	return -1;
}

int WinUtil::setButtonPressed(int nID, bool bPressed /* = true */) {
	if (nID == -1)
		return -1;
	if (!MainFrame::getMainFrame()->getToolBar().IsWindow())
		return -1;

	MainFrame::getMainFrame()->getToolBar().CheckButton(nID, bPressed);
	return 0;
}

void WinUtil::ShowBalloonTip(LPCTSTR szMsg, LPCTSTR szTitle, DWORD dwInfoFlags) {
  BallonPopup* p = new BallonPopup();
  p->Title = szTitle;
  p->Message = szMsg;
  p->Icon = dwInfoFlags;
  PostMessage(mainWnd, WM_SPEAKER, MainFrame::SHOW_POPUP, (LPARAM)p);
}

void WinUtil::showPrivateMessageTrayIcon() {
  if (BOOLSETTING(MINIMIZE_TRAY) && !g_RunningUnderWine) {
    ::PostMessage(mainWnd, WM_SPEAKER, MainFrame::SET_PM_TRAY_ICON, NULL);
  }										
}

bool WinUtil::isAppMinimized() {
  return MainFrame::getMainFrame()->getAppMinimized();
}

HWND WinUtil::MDIGetActive() {
  return MainFrame::getMainFrame()->MDIGetActive();
}

void WinUtil::MDIActivate(HWND hWnd) {
  MainFrame::getMainFrame()->MDIActivate(hWnd);
}

// This is GPLed, and copyright (mostly) my anonymous friend
// - Todd Pederzani
string WinUtil::getItunesSpam(HWND playerWnd /*= NULL*/) {
	// If it's not running don't even bother...
	if(playerWnd != NULL) {
		// Pointer
		IiTunes *iITunes;
	
		// Load COM library
		CoInitialize(NULL);

		// Others
		StringMap params;

		// note - CLSID_iTunesApp and IID_IiTunes are defined in iTunesCOMInterface_i.c
		//Create an instance of the top-level object.  iITunes is an interface pointer to IiTunes.  (weird capitalization, but that's how Apple did it)
		if (SUCCEEDED(::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&iITunes))) {
			long length(0), elapsed;

			//pTrack is a pointer to the track.  This gets passed to other functions to get track data.  wasTrack lets you check if the track was grabbed.
			IITTrack *pTrack;
			//Sanity check -- should never fail if CoCreateInstance succeeded.  You may want to use this for debug output if it does ever fail.
			if (SUCCEEDED(iITunes->get_CurrentTrack(&pTrack)) && pTrack != NULL) {
				//Get album, then call ::COLE2T() to convert the text to array
				BSTR album;
				pTrack->get_Album(&album);
				if (album != NULL) {
					::COLE2T Album(album);
					params["album"] = Text::fromT(Album.m_szBuffer);
				}

				//Same for artist
				BSTR artist;
				pTrack->get_Artist(&artist);
				if(artist != NULL) {
					::COLE2T Artist(artist);
					params["artist"] = Text::fromT(Artist.m_szBuffer);
				}

				//Track name (get_Name is inherited from IITObject, of which IITTrack is derived)
				BSTR name;
				pTrack->get_Name(&name);
				if(name != NULL) {
					::COLE2T Name(name);
					params["title"] = Text::fromT(Name.m_szBuffer);
				}

				// Genre
				BSTR genre;
				pTrack->get_Genre(&genre);
				if(genre != NULL) {
					::COLE2T Genre(name);
					params["genre"] = Text::fromT(Genre.m_szBuffer);
				}

				//Total song time
				pTrack->get_Duration(&length);
				if (length > 0) { params["length"] = Text::fromT(Util::formatSeconds(length)); } // <--- once more with feeling

				//Bitrate
				long bitrate;
				pTrack->get_BitRate(&bitrate);
				if (bitrate > 0) { params["bitrate"] = Util::toString(bitrate) + "kbps"; } //<--- I'm not gonna play those games.  Mind games.  Board games.  I'm like, come on fhqugads...

				//Frequency
				long frequency;
				pTrack->get_SampleRate(&frequency);
				if (frequency > 0) { params["frequency"] = Util::toString(frequency/1000) + "kHz"; }

				//Year
				long year;
				pTrack->get_Year(&year);
				if (year > 0) { params["year"] = Util::toString(year); }
			
				//Size
				long size;
				pTrack->get_Size(&size);
				if (size > 0) { params["size"] = Util::formatBytes(size); }

				//Release (decrement reference count to 0) track object so it can unload and free itself; otherwise, it's locked in memory.
				pTrack->Release();
			}

			//Player status (stopped, playing, FF, rewind)
			ITPlayerState pStatus;
			iITunes->get_PlayerState(&pStatus);
			if (pStatus == ITPlayerStateStopped) {
				params["state"] = "stopped";
			} else if (pStatus == ITPlayerStatePlaying) {
				params["state"] = "playing";
			}

			//Player position (in seconds, you'll want to convert for your output)
			iITunes->get_PlayerPosition(&elapsed);
			if(elapsed > 0) {
				params["elapsed"] = Text::fromT(Util::formatSeconds(elapsed));
				int intPercent;
				if (length > 0 ) {
					intPercent = elapsed * 100 / length;
				} else {
					length = 0;
					intPercent = 0;
				}
				params["percent"] = Util::toString(intPercent) + "%";
				int numFront = min(max(intPercent / 10, 0), 10),
					numBack = min(max(10 - 1 - numFront, 0), 10);
				string inFront = string(numFront, '-'),
				   inBack = string(numBack, '-');
				params["bar"] = "[" + inFront + (elapsed > 0 ? "|" : "-") + inBack + "]";
			}

			//iTunes version
			BSTR version;
			iITunes->get_Version(&version);
			if(version != NULL) {
				::COLE2T iVersion(version);
				params["version"] = Text::fromT(iVersion.m_szBuffer);
			}

			//Release (decrement reference counter to 0) IiTunes object so it can unload and free itself; otherwise, it's locked in memory
			iITunes->Release();
		}

		//unload COM library -- this is also essential to prevent leaks and to keep it working the next time.
		CoUninitialize();

		// If there is something in title, we have at least partly succeeded 
		if(!params["title"].empty()) {
			return Util::formatParams(SETTING(ITUNES_FORMAT), params, false);
		} else {
			return "no_media";
		}
	} else {
		return "";
	}
}

// mpc = mplayerc = MediaPlayer Classic
// liberally inspired (copied with changes) by the GPLed application mpcinfo by Gabest
string WinUtil::getMPCSpam() {
	StringMap params;
	bool success = false;
	CComPtr<IRunningObjectTable> pROT;
	CComPtr<IEnumMoniker> pEM;
	CComQIPtr<IFilterGraph> pFG;
	if(GetRunningObjectTable(0, &pROT) == S_OK && pROT->EnumRunning(&pEM) == S_OK) {
		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);
		for(CComPtr<IMoniker> pMoniker; pEM->Next(1, &pMoniker, NULL) == S_OK; pMoniker = NULL) {
			LPOLESTR pDispName = NULL;
			if(pMoniker->GetDisplayName(pBindCtx, NULL, &pDispName) != S_OK)
				continue;
			wstring strw(pDispName);
			CComPtr<IMalloc> pMalloc;
			if(CoGetMalloc(1, &pMalloc) != S_OK)
				continue;
			pMalloc->Free(pDispName);
			// Prefix string literals with the L character to indicate a UNCODE string.
			if(strw.find(L"(MPC)") == wstring::npos)
				continue;
			CComPtr<IUnknown> pUnk;
			if(pROT->GetObject(pMoniker, &pUnk) != S_OK)
				continue;
			pFG = pUnk;
			if(!pFG)
				continue;
			success = true;
			break;
		}

		if (success) {
			// file routine (contains size routine)
			CComPtr<IEnumFilters> pEF;
			if(pFG->EnumFilters(&pEF) == S_OK) {
				// from the file routine
				ULONG cFetched = 0;
				for(CComPtr<IBaseFilter> pBF; pEF->Next(1, &pBF, &cFetched) == S_OK; pBF = NULL) {
					if(CComQIPtr<IFileSourceFilter> pFSF = pBF) {
						LPOLESTR pFileName = NULL;
						AM_MEDIA_TYPE mt;
						if(pFSF->GetCurFile(&pFileName, &mt) == S_OK) {
							// second parameter is a AM_MEDIA_TYPE structure, which contains codec info
							//										LPSTR thisIsBad = (LPSTR) pFileName;
							LPSTR test = new char[1000];
							if (test != NULL) {
								WideCharToMultiByte(CP_ACP, 0, pFileName, -1, test, 1000, NULL, NULL);
								string filename(test);
								params["filename"] = Util::getFileName(filename); //otherwise fully qualified
								params["title"] = params["filename"].substr(0, params["filename"].size() - 4);
								params["size"] = Util::formatBytes(File::getSize(filename));
							}
							delete [] test;
							CoTaskMemFree(pFileName);
							// alternative to FreeMediaType(mt)
							// provided by MSDN DirectX 9 help page for FreeMediaType
							if (mt.cbFormat != 0)
							{
								CoTaskMemFree((PVOID)mt.pbFormat);
								mt.cbFormat = 0;
								mt.pbFormat = NULL;
							}
							if (mt.pUnk != NULL)
							{
								// Unecessary because pUnk should not be used, but safest.
								mt.pUnk->Release();
								mt.pUnk = NULL;
							}
							// end provided by MSDN
							break;
						}
					}
				}
			}

			// paused / stopped / running?
			CComQIPtr<IMediaControl> pMC;
			OAFilterState fs;
			int state = 0;
			if((pMC = pFG) && (pMC->GetState(0, &fs) == S_OK)) {
				switch(fs) {
					case State_Running:
						params["state"] = "playing";
						state = 1;
						break;
					case State_Paused:
						params["state"] = "paused";
						state = 3;
						break;
					case State_Stopped:
						params["state"] = "stopped";
						state = 0;
				};
			}

			// position routine
			CComQIPtr<IMediaSeeking> pMS = pFG;
			REFERENCE_TIME pos, dur;
			if((pMS->GetCurrentPosition(&pos) == S_OK) && (pMS->GetDuration(&dur) == S_OK)) {
				params["elapsed"] =  Text::fromT(Util::formatSeconds(pos/10000000));
				params["length"] =  Text::fromT(Util::formatSeconds(dur/10000000));
				int intPercent = 0;
				if (dur != 0)
					intPercent = (int) (pos * 100 / dur);
				params["percent"] = Util::toString(intPercent) + "%";
				int numFront = min(max(intPercent / 10, 0), 10),
					numBack = min(max(10 - 1 - numFront, 0), 10);
				string inFront = string(numFront, '-'),
					   inBack = string(numBack, '-');
				params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
			}
			/*
			*	"+me watches: %[filename] (%[elapsed]/%[length]) Size: %[size]"
			*	from mpcinfo.txt
			*	"+me is playing %[filename] %[size] (%[elapsed]/%[length]) Media Player Classic %[version]"
			*	from http://www.faireal.net/articles/6/25/
			*/
		}
	}

	if (success) {
		return Util::formatParams(SETTING(MPLAYERC_FORMAT), params, false);
	} else {
		 return "";
	}
}

// This works for WMP 9+, but it's little slow, but hey we're talking about an MS app here, so what can you expect from the remote support for it...
string WinUtil::getWMPSpam(HWND playerWnd /*= NULL*/) {
        // If it's not running don't even bother...
        if(playerWnd != NULL) {
                // Load COM
                CoInitialize(NULL);

		// Pointers 
		CComPtr<IWMPPlayer>					Player;
		CComPtr<IAxWinHostWindow>			Host;
		CComPtr<IObjectWithSite>			HostObj;
		CComObject<WMPlayerRemoteApi>		*WMPlayerRemoteApiCtrl = NULL;

		// Other
		HRESULT								hresult;
		CAxWindow *DummyWnd;
		StringMap params;

		// Create hidden window to host the control (if there just was other way to do this... as CoCreateInstance has no access to the current running instance)
		AtlAxWinInit();
		DummyWnd = new CAxWindow();
		hresult = DummyWnd? S_OK : E_OUTOFMEMORY;

		if(SUCCEEDED(hresult)) {
			DummyWnd->Create(mainWnd, NULL, NULL, WS_CHILD | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU);
			hresult = ::IsWindow(DummyWnd->m_hWnd)? S_OK : E_FAIL;
		}
		
		// Set WMPlayerRemoteApi
		if(SUCCEEDED(hresult)) {
			hresult = DummyWnd->QueryHost(IID_IObjectWithSite, (void **)&HostObj);
			hresult = HostObj.p? hresult : E_FAIL;

			if(SUCCEEDED(hresult)) {
				hresult = CComObject<WMPlayerRemoteApi>::CreateInstance(&WMPlayerRemoteApiCtrl);
				if(WMPlayerRemoteApiCtrl) {
					WMPlayerRemoteApiCtrl->AddRef();
				} else {
					hresult = E_POINTER;
				}
			}

			if(SUCCEEDED(hresult)) {
				hresult = HostObj->SetSite((IWMPRemoteMediaServices *)WMPlayerRemoteApiCtrl);
			}
		}
		
		if(SUCCEEDED(hresult)) {
			hresult = DummyWnd->QueryHost(&Host);
			hresult = Host.p? hresult : E_FAIL;
		}

		// Create WMP Control 
		if(SUCCEEDED(hresult)) {
			hresult = Host->CreateControl(CComBSTR(L"{6BF52A52-394A-11d3-B153-00C04F79FAA6}"), DummyWnd->m_hWnd, 0);
		}
	
		// Now we can finally start to interact with WMP, after we successfully get the "Player"
		if(SUCCEEDED(hresult)) {
			hresult = DummyWnd->QueryControl(&Player);
			hresult = Player.p? hresult : E_FAIL;
		}

		// We've got this far now the grande finale, get the metadata :)
		if(SUCCEEDED(hresult)) {
			CComPtr<IWMPMedia>		Media;
			CComPtr<IWMPControls>	Controls;

			if(SUCCEEDED(Player->get_currentMedia(&Media)) && Media.p != NULL) {
				Player->get_controls(&Controls);

				// Windows Media Player version
				CComBSTR bstrWMPVer;
				Player->get_versionInfo(&bstrWMPVer);
				if(bstrWMPVer != NULL) {
					::COLE2T WMPVer(bstrWMPVer);
					params["fullversion"] = Text::fromT(WMPVer.m_szBuffer);
					params["version"] = params["fullversion"].substr(0, params["fullversion"].find("."));
				}

				// Pre-formatted status message from Windows Media Player
				CComBSTR bstrWMPStatus;
				Player->get_status(&bstrWMPStatus);
				if(bstrWMPStatus != NULL) {
					::COLE2T WMPStatus(bstrWMPStatus);
					params["status"] = Text::fromT(WMPStatus.m_szBuffer);
				}

				// Name of the currently playing media
				CComBSTR bstrMediaName;
				Media->get_name(&bstrMediaName);
				if(bstrMediaName != NULL) {
					::COLE2T MediaName(bstrMediaName);
					params["title"] = Text::fromT(MediaName.m_szBuffer);
				}

				// How much the user has already played 
				// I know this is later duplicated with get_currentPosition() for percent and bar, but for some reason it's overall faster this way 
				CComBSTR bstrMediaPosition;
				Controls->get_currentPositionString(&bstrMediaPosition);
				if(bstrMediaPosition != NULL) {
					::COLE2T MediaPosition(bstrMediaPosition);
					params["elapsed"] = Text::fromT(MediaPosition.m_szBuffer);
				}

				// Full duratiuon of the media
				// I know this is later duplicated with get_duration() for percent and bar, but for some reason it's overall faster this way 
				CComBSTR bstrMediaDuration;
				Media->get_durationString(&bstrMediaDuration);
				if(bstrMediaDuration != NULL) {
					::COLE2T MediaDuration(bstrMediaDuration);
					params["length"] = Text::fromT(MediaDuration.m_szBuffer);
				}

				// Name of the artist (use Author as secondary choice)
				CComBSTR bstrArtistName;
				Media->getItemInfo(CComBSTR(_T("WM/AlbumArtist")), &bstrArtistName);
				if(bstrArtistName != NULL) {
					::COLE2T ArtistName(bstrArtistName);
					params["artist"] = Text::fromT(ArtistName.m_szBuffer);
				} else {
					Media->getItemInfo(CComBSTR(_T("Author")), &bstrArtistName);
					if(bstrArtistName != NULL) {
						::COLE2T ArtistName(bstrArtistName);
						params["artist"] = Text::fromT(ArtistName.m_szBuffer);
					}
				}

				// Name of the album
				CComBSTR bstrAlbumTitle;
				Media->getItemInfo(CComBSTR(_T("WM/AlbumTitle")), &bstrAlbumTitle);
				if(bstrAlbumTitle != NULL) {
					::COLE2T AlbumName(bstrAlbumTitle);
					params["album"] = Text::fromT(AlbumName.m_szBuffer);
				}

				// Genre of the media
				CComBSTR bstrMediaGen;
				Media->getItemInfo(CComBSTR(_T("WM/Genre")), &bstrMediaGen);
				if(bstrMediaGen != NULL) {
					::COLE2T MediaGen(bstrMediaGen);
					params["genre"] = Text::fromT(MediaGen.m_szBuffer);
				}

				// Year of publiciation
				CComBSTR bstrMediaYear;
				Media->getItemInfo(CComBSTR(_T("WM/Year")), &bstrMediaYear);
				if(bstrMediaYear != NULL) {
					::COLE2T MediaYear(bstrMediaYear);
					params["year"] = Text::fromT(MediaYear.m_szBuffer);
				} else {
					Media->getItemInfo(CComBSTR(_T("ReleaseDateYear")), &bstrMediaYear);
					if(bstrMediaYear != NULL) {
						::COLE2T MediaYear(bstrMediaYear);
						params["year"] = Text::fromT(MediaYear.m_szBuffer);
					}
				}

				// Bitrate, displayed as Windows Media Player displays it
				CComBSTR bstrMediaBitrate;
				Media->getItemInfo(CComBSTR(_T("Bitrate")), &bstrMediaBitrate);
				if(bstrMediaBitrate != NULL) {
					::COLE2T MediaBitrate(bstrMediaBitrate);
					double BitrateAsKbps = (Util::toDouble(Text::fromT(MediaBitrate.m_szBuffer))/1000);
					params["bitrate"] = Util::toString(int(BitrateAsKbps)) + "kbps";
				}

				// Size of the file
				CComBSTR bstrMediaSize;
				Media->getItemInfo(CComBSTR(_T("Size")), &bstrMediaSize);
				if(bstrMediaSize != NULL) {
					::COLE2T MediaSize(bstrMediaSize);
					params["size"] = Util::formatBytes(Text::fromT(MediaSize.m_szBuffer));
				}

				// Users rating for this media
				CComBSTR bstrUserRating;
				Media->getItemInfo(CComBSTR(_T("UserRating")), &bstrUserRating);
				if(bstrUserRating != NULL) {
					if(bstrUserRating == "0") {
						params["rating"] = "unrated";
					} else if(bstrUserRating == "1") {
						params["rating"] = "*";
					} else if(bstrUserRating == "25") {
						params["rating"] = "* *";
					} else if(bstrUserRating == "50") {
						params["rating"] = "* * *";
					} else if(bstrUserRating == "75") {
						params["rating"] = "* * * *";
					} else if(bstrUserRating == "99") {
						params["rating"] = "* * * * *";
					} else {
						params["rating"] = "";
					}
				}

				// Bar & percent
				double elapsed;
				double length;
				Controls->get_currentPosition(&elapsed);
				Media->get_duration(&length);
				if(elapsed > 0) {
					int intPercent;
					if (length > 0 ) {
						intPercent = int(elapsed) * 100 / int(length);
					} else {
						length = 0;
						intPercent = 0;
					}
					params["percent"] = Util::toString(intPercent) + "%";
					int numFront = min(max(intPercent / 10, 0), 10),
						numBack = min(max(10 - 1 - numFront, 0), 10);
					string inFront = string(numFront, '-'),
						inBack = string(numBack, '-');
					params["bar"] = "[" + inFront + (elapsed > 0 ? "|" : "-") + inBack + "]";
				} else {
					params["percent"] = "0%";
					params["bar"] = "[|---------]";
				}
			}
		}

                // Release WMPlayerRemoteApi, if it's there
                if(WMPlayerRemoteApiCtrl) {
                        WMPlayerRemoteApiCtrl->Release();
                }

                // Destroy the hoster window, and unload COM
                DummyWnd->DestroyWindow();
                delete DummyWnd;
                CoUninitialize();

                // If there is something in title, we have at least partly succeeded
                if(!params["title"].empty()) {
                        return Util::formatParams(SETTING(WMP_FORMAT), params, false);
                } else {
                        return "no_media";
                }
        } else {
                return "";
        }
}

string WinUtil::getWinampSpam(HWND playerWnd) {
	if (playerWnd) {
		StringMap params;
		int waVersion = SendMessage(playerWnd,WM_USER, 0, IPC_GETVERSION),
			majorVersion, minorVersion;
			majorVersion = waVersion >> 12;
		if (((waVersion & 0x00F0) >> 4) == 0) {
			minorVersion = ((waVersion & 0x0f00) >> 8) * 10 + (waVersion & 0x000f);
		} else {
			minorVersion = ((waVersion & 0x00f0) >> 4) * 10 + (waVersion & 0x000f);
		}
		params["version"] = Util::toString(majorVersion + minorVersion / 100.0);
		int state = SendMessage(playerWnd,WM_USER, 0, IPC_ISPLAYING);
		switch (state) {
			case 0: params["state"] = "stopped";
				break;
			case 1: params["state"] = "playing";
				break;
			case 3: params["state"] = "paused";
		};
		TCHAR titleBuffer[2048];
		int buffLength = sizeof(titleBuffer);
		GetWindowText(playerWnd, titleBuffer, buffLength);
		tstring title = titleBuffer;
		params["rawtitle"] = Text::fromT(title);
		// there's some winamp bug with scrolling. wa5.09x and 5.1 or something.. see:
		// http://forums.winamp.com/showthread.php?s=&postid=1768775#post1768775
		int starpos = title.find(_T("***"));
		if (starpos >= 1) {
			tstring firstpart = title.substr(0, starpos - 1);
			if (firstpart == title.substr(title.size() - firstpart.size(), title.size())) {
				// fix title
				title = title.substr(starpos, title.size());
			}
		}
		// fix the title if scrolling is on, so need to put the stairs to the end of it
		tstring titletmp = title.substr(title.find(_T("***")) + 2, title.size());
		title = titletmp + title.substr(0, title.size() - titletmp.size());
		title = title.substr(title.find(_T('.')) + 2, title.size());
		if (title.rfind(_T('-')) != string::npos) {
			params["title"] = Text::fromT(title.substr(0, title.rfind(_T('-')) - 1));
		}
		int curPos = SendMessage(playerWnd,WM_USER, 0, IPC_GETOUTPUTTIME);
		int length = SendMessage(playerWnd,WM_USER, 1, IPC_GETOUTPUTTIME);
		if (curPos == -1) {
			curPos = 0;
		} else {
			curPos /= 1000;
		}
		int intPercent;
		if (length > 0 ) {
			intPercent = curPos * 100 / length;
		} else {
			length = 0;
			intPercent = 0;
		}
		params["percent"] = Util::toString(intPercent) + "%";
		params["elapsed"] = Text::fromT(Util::formatSeconds(curPos, true));
		params["length"] = Text::fromT(Util::formatSeconds(length, true));
		int numFront = min(max(intPercent / 10, 0), 10),
			numBack = min(max(10 - 1 - numFront, 0), 10);
		string inFront = string(numFront, '-'),
			inBack = string(numBack, '-');
		params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
		int waSampleRate = SendMessage(playerWnd,WM_USER, 0, IPC_GETINFO),
			waBitRate = SendMessage(playerWnd,WM_USER, 1, IPC_GETINFO),
			waChannels = SendMessage(playerWnd,WM_USER, 2, IPC_GETINFO);
		params["bitrate"] = Util::toString(waBitRate) + "kbps";
		params["sample"] = Util::toString(waSampleRate) + "kHz";
		// later it should get some improvement:
		string waChannelName;
		switch (waChannels) {
			case 2:
				waChannelName = "stereo";
				break;
			case 6:
				waChannelName = "5.1 surround";
				break;
			default:
				waChannelName = "mono";
		}
		params["channels"] = waChannelName;
		//params["channels"] = (waChannels==2?"stereo":"mono"); // 3+ channels? 0 channels?
		return Util::formatParams(SETTING(WINAMP_FORMAT), params, false);
	} else {
		return "";
	}
}
/**
 * @file
 * $Id: WinUtil.cpp,v 1.25.2.6 2008/12/21 14:29:37 alexey Exp $
 */
