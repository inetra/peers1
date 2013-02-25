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

#include "MainFrm.h"

#include "../peers/HubFrameFactory.h"
#include "../peers/SearchFrmFactory.h"
//[-]PPA #include "PublicHubsFrm.h"
#include "PropertiesDlg.h"
#include "UsersFrame.h"
#include "DirectoryListingFrm.h"
#include "RecentsFrm.h"
#include "FavoritesFrm.h"
#include "NotepadFrame.h"
#include "QueueFrame.h"
#include "SpyFrame.h"
#include "FinishedFrame.h"
#include "ADLSearchFrame.h"
#include "FinishedULFrame.h"
#include "TextFrame.h"
#include "StatsFrame.h"
#include "WaitingUsersFrame.h"
#include "HashProgressDlg.h"
#ifdef PPA_INCLUDE_UPNP
#include "UPnP.h"
#endif
#include "../peers/PrivateFrameFactory.h"
#include "CDMDebugFrame.h"
#include "InputBox.h"
#include "PopupManager.h"
#include "AGEmotionSetup.h"
#include "Winamp.h"
#include "Players.h"
#include "iTunesCOMInterface.h"
#include "ToolbarManager.h"
#include "AboutDlg.h"
#include "SplashWindow.h"
#include "UploadPage.h"

#include "../client/ConnectionManager.h"
#include "../client/UploadManager.h"
#include "../client/WebServerManager.h"

#ifdef PPA_INCLUDE_CHECK_UPDATE
#include "../peers/AutoUpdate.h"
#include "../peers/UpdateRestartConfirmDlg.h"
#include "../peers/PeersUtils.h"
#include "../client/peers/IpBlocksLoader.h"
#include "../client/peers/ConfigurationPatcher.h"
#include "../client/PGLoader.h"
#endif

#include "../peers/ControlAdjuster.h"
#include "../peers/EgoPhoneLauncher.h"
#include "../peers/InvokeLater.h"
#include "../peers/AdviceFrame.h"
#include "../peers/AutodetectNetwork.h"

#ifdef PPA_INCLUDE_CHECK_UPDATE
enum {
	UPDATE_MODE_APP,
#ifdef INCLUDE_PROVIDE_SELECTION
	UPDATE_MODE_IP_BLOCKS,
#endif
	UPDATE_MODE_CONFIGURATION
};
#endif

MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
uint64_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;
CAGEmotionSetup* g_pEmotionsSetup = NULL;

MainFrame::MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
lastUp(0), lastDown(0), stopperThread(NULL), m_OLDPriorityClass(0),
exitToUpdate(false),
splitterResizeActive(false),
#ifdef PPA_INCLUDE_CHECK_UPDATE
updatesChecker(new HttpConnection(PeersUtils::getUserAgent())),
downloader(NULL),
#endif
pinger(new HttpConnection(PeersUtils::getUserAgent())),
closeEnabled(false),
closing(false), awaybyminimize(false), missedAutoConnect(false), lastTTHdir(Util::emptyStringT), 
bTrayIcon(false), bAppMinimized(false), bIsPM(false),
#ifdef PPA_INCLUDE_UPNP
UPnP_TCPConnection(NULL), UPnP_UDPConnection(NULL),
#endif
m_searchBar(this)
{ 
		memzero(statusSizes, sizeof(statusSizes));
		anyMF = this;
}

MainFrame::~MainFrame() {
	m_CmdBar.m_hImageList = NULL;

	delete g_pEmotionsSetup;
	g_pEmotionsSetup = NULL;

	WinUtil::uninit();
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;

	while( (wnd=::GetWindow(mf->m_hWndMDIClient, GW_CHILD)) != NULL) {
		if(wnd == wnd2)
			Sleep(100);
		else { 
			::PostMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	mf->PostMessage(WM_CLOSE);	
	return 0;
}

class ListMatcher : public Thread {
public:
	ListMatcher(StringList files_) : files(files_) {

	}
	virtual int run() {
		for(StringIter i = files.begin(); i != files.end(); ++i) {
			UserPtr u = DirectoryListing::getUserFromFilename(*i);
			if(!u)
				continue;
			DirectoryListing dl(u);
			try {
				dl.loadFile(*i);
				const size_t BUF_SIZE = STRING(MATCHED_FILES).size() + 16;
				AutoArray<char> tmp(BUF_SIZE);
				snprintf(tmp, BUF_SIZE, CSTRING(MATCHED_FILES), QueueManager::getInstance()->matchListing(dl));
				LogManager::getInstance()->message(u->getFirstNick() + ": " + string(tmp));
			} catch(const Exception&) {

			}
		}
		delete this;
		return 0;
	}
	StringList files;
};

LRESULT MainFrame::onMatchAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ListMatcher* matcher = new ListMatcher(File::findFiles(Util::getListPath(), "*.xml*"));
	try {
		matcher->start();
	} catch(const ThreadException&) {
		///@todo add error message
		delete matcher;
	}
	
	return 0;
}

static AdviceFrame* isAdviceWindowOpen() {
	vector<MDIContainer::Window> windows = MDIContainer::list();
	for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
		AdviceFrame *adviceFrame = dynamic_cast<AdviceFrame*>(*i);
		if (adviceFrame) {
			return adviceFrame;
		}
	}
	return NULL;
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    delete g_pEmotionsSetup;
    g_pEmotionsSetup = NULL;
	g_pEmotionsSetup = new CAGEmotionSetup;
	if ((g_pEmotionsSetup == NULL)||
		(!g_pEmotionsSetup->Create())){
		dcassert(FALSE);
		return -1;
	}

	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);
	WebServerManager::getInstance()->addListener(this);
	if(BOOLSETTING(WEBSERVER)) {
		try {
			WebServerManager::getInstance()->Start();
		} catch(const Exception& e) {
			MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
	}
	WinUtil::init(m_hWnd);
	WinUtil::allowUserTOSSetting();

	trayMessage = RegisterWindowMessage(_T("TaskbarCreated"));

	TimerManager::getInstance()->start();

	// Set window name
#ifndef _DEBUG
	SetWindowText(_T(APPNAME) _T(" ") _T(VERSIONSTRING));
#else
	SetWindowText(_T(APPNAME) _T(" - DEBUG"));
#endif
	// Load images
	// create command bar window
	hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);

	m_hMenu = WinUtil::mainMenu;

	hShutdownIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SHUTDOWN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	images.CreateFromImage(IDB_TOOLBAR, 16, 16, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	
	m_CmdBar.AttachMenu(m_hMenu);
	m_CmdBar.m_hImageList = images;
	m_CmdBar.m_arrCommand.Add(ID_FILE_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_RECONNECT);
	m_CmdBar.m_arrCommand.Add(IDC_FOLLOW);
	m_CmdBar.m_arrCommand.Add(IDC_RECENTS);
	m_CmdBar.m_arrCommand.Add(IDC_FAVORITES);
	m_CmdBar.m_arrCommand.Add(IDC_FAVUSERS);
	m_CmdBar.m_arrCommand.Add(IDC_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED);
	m_CmdBar.m_arrCommand.Add(IDC_UPLOAD_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED_UL);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_FILE_ADL_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_SEARCH_SPY);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_FILE_LIST);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	m_CmdBar.m_arrCommand.Add(IDC_NOTEPAD);
	m_CmdBar.m_arrCommand.Add(IDC_NET_STATS);
	m_CmdBar.m_arrCommand.Add(IDC_CDMDEBUG_WINDOW);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_CASCADE);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_HORZ);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_VERT);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_MINIMIZE_ALL);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_RESTORE_ALL);
	m_CmdBar.m_arrCommand.Add(ID_GET_TTH);	
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_MY_LIST);
	m_CmdBar.m_arrCommand.Add(IDC_MATCH_ALL);
	m_CmdBar.m_arrCommand.Add(IDC_REFRESH_FILE_LIST);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_DOWNLOADS);
	m_CmdBar.m_arrCommand.Add(ID_FILE_QUICK_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_APP_EXIT);
	m_CmdBar.m_arrCommand.Add(IDC_HASH_PROGRESS);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_ARRANGE);
	m_CmdBar.m_arrCommand.Add(ID_APP_ABOUT);
	m_CmdBar.m_arrCommand.Add(IDC_HELP_HOMEPAGE);
	m_CmdBar.m_arrCommand.Add(IDC_SITES_FLYLINK_TRAC);
	m_CmdBar.m_arrCommand.Add(IDC_HELP_DISCUSS);
	m_CmdBar.m_arrCommand.Add(IDC_HELP_DONATE);
	m_CmdBar.m_arrCommand.Add(IDC_GUIDES);
	m_CmdBar.m_arrCommand.Add(IDC_HELP_GEOIPFILE);	

	// remove old menu
	SetMenu(NULL);

	tbarcreated = false;
	HWND hWndToolBar = createToolbar();
	HWND hWndWinampBar = createWinampToolbar();
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
        if (BOOLSETTING(MENU_ADVANCED)) {
          AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
          AddSimpleReBarBand(hWndWinampBar, NULL, TRUE);
        }
	CreateSimpleStatusBar();

	CReBarCtrl rebar = m_hWndToolBar;
	ToolbarManager::getInstance()->applyTo(rebar, "MainToolBar");

	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	int w[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ctrlStatus.SetParts(10, w);
    HDC l_dc = ::GetDC(ctrlStatus.m_hWnd); //[+]PPA
	statusSizes[0] = WinUtil::getTextWidth(TSTRING(AWAY), l_dc); // for "AWAY" segment
	::ReleaseDC(ctrlStatus.m_hWnd, l_dc); //[+]PPA

	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);
        ControlAdjuster::fixToolInfoSize(ti);
	ctrlLastLines.AddTool(&ti);
	ctrlLastLines.SetDelayTime(TTDT_AUTOPOP, 15000);

	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	WinUtil::mdiClient = m_hWndMDIClient;

	ctrlTab.Create(m_hWnd, rcDefault);
	WinUtil::tabCtrl = &ctrlTab;
        m_peersToolbar.Create(m_hWnd);
		m_searchBar.Create(m_hWnd);
        m_footer.Create(m_hWnd);
	transferView.Create(m_footer);
        m_hubMessages.Create(m_footer);
        m_footer.setTransferView(&transferView);
        m_footer.setHubMessageControl(&m_hubMessages);
        m_footer.SetSplitterPanes(transferView, m_hubMessages);

	SetSplitterPanes(m_hWndMDIClient, m_footer);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
        m_nProportionalPos = SETTING(TRANSFER_SPLIT_SIZE);
        transferView.setProportionalPos(m_nProportionalPos);
        transferView.setConstructed(true);
	UIAddToolBar(hWndToolBar);
	UIAddToolBar(hWndWinampBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TRANSFER_VIEW, 1);
	UISetCheck(ID_TOGGLE_TOOLBAR, 1);

	UISetCheck(IDC_TRAY_LIMITER, BOOLSETTING(THROTTLE_ENABLE));
	UISetCheck(IDC_TOPMOST, BOOLSETTING(TOPMOST));
	UISetCheck(IDC_LOCK_TOOLBARS, BOOLSETTING(LOCK_TOOLBARS));
	
	if(BOOLSETTING(TOPMOST)) toggleTopmost();
	if(BOOLSETTING(LOCK_TOOLBARS)) toggleLockToolbars();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	trayMenu.CreatePopupMenu();
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_SHOW, CTSTRING(MENU_SHOW));
	trayMenu.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_LIMITER, CTSTRING(TRAY_LIMITER));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
	trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));
	trayMenu.SetMenuDefaultItem(IDC_TRAY_SHOW);

	tbMenu.CreatePopupMenu();
	tbMenu.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
	tbMenu.AppendMenu(MF_STRING, ID_TOGGLE_TOOLBAR, CTSTRING(TOGGLE_TOOLBAR));
	tbMenu.AppendMenu(MF_SEPARATOR);
	tbMenu.AppendMenu(MF_STRING, IDC_LOCK_TOOLBARS, CTSTRING(LOCK_TOOLBARS));

	if(SETTING(PROTECT_START) && hasPassword()) {
		LineDlg dlg;
		dlg.description = TSTRING(PASSWORD_DESC);
		dlg.title = TSTRING(PASSWORD_TITLE);
		dlg.password = true;
		dlg.disable = true;
		if(dlg.DoModal(m_hWnd) == IDOK ) {
			tstring tmp = dlg.line;
			TigerTree mytth(TigerTree::calcBlockSize(tmp.size(), 1));
			mytth.update(tmp.c_str(), tmp.size());
			mytth.finalize();
			if(mytth.getRoot().toBase32().c_str() != SETTING(PASSWORD)) {
				ExitProcess(1);
			}
		}
	} 

#ifdef PPA_INCLUDE_CHECK_UPDATE
	updatesChecker->addListener(this);
	if (AutoUpdate::allowOnStart()) {
          StringMap params;
          params["message"] = "Checking updates on startup";
          LOG(LogManager::SYSTEM, params);
		  updatesChecker->downloadFile(string(VERSIONFILE) + "?V=" + Util::toString(BUILDID) + "&M=S&NICK=" + SETTING(NICK) + "&CID=" + SETTING(PRIVATE_ID));
	}
	if (AutoUpdate::allowPeriodic()) {
	  SetTimer(TIMER_CHECK_UPDATE, AutoUpdate::getUpdateCheckInterval() * 1000);
	}
#endif

	if (!SETTING(HTTP_PING_ADDRESS).empty()) {
		pinger->downloadFile(SETTING(HTTP_PING_ADDRESS));
		if (SETTING(HTTP_PING_INTERVAL) > 60*1000) {
			SetTimer(PINGER_TIMER, SETTING(HTTP_PING_INTERVAL));
		}
	}

	if(BOOLSETTING(OPEN_ADVICE)) PostMessage(WM_COMMAND, IDC_ADVICE_WINDOW);
	if(BOOLSETTING(OPEN_FAVORITE_HUBS)) PostMessage(WM_COMMAND, IDC_FAVORITES);
	if(BOOLSETTING(OPEN_FAVORITE_USERS)) PostMessage(WM_COMMAND, IDC_FAVUSERS);
	if(BOOLSETTING(OPEN_QUEUE)) PostMessage(WM_COMMAND, IDC_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_DOWNLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED);
	if(BOOLSETTING(OPEN_WAITING_USERS)) PostMessage(WM_COMMAND, IDC_UPLOAD_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_UPLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED_UL);
	if(BOOLSETTING(OPEN_SEARCH_SPY)) PostMessage(WM_COMMAND, IDC_SEARCH_SPY);
	if(BOOLSETTING(OPEN_NETWORK_STATISTICS)) PostMessage(WM_COMMAND, IDC_NET_STATS);
	if(BOOLSETTING(OPEN_NOTEPAD)) PostMessage(WM_COMMAND, IDC_NOTEPAD);
	if(BOOLSETTING(OPEN_CDMDEBUG)) PostMessage(WM_COMMAND, IDC_CDMDEBUG_WINDOW);

	if(!BOOLSETTING(SHOW_STATUSBAR)) PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);
	if(!BOOLSETTING(SHOW_TOOLBAR)) PostMessage(WM_COMMAND, ID_VIEW_TOOLBAR);
	if(!BOOLSETTING(SHOW_TRANSFERVIEW))	PostMessage(WM_COMMAND, ID_VIEW_TRANSFER_VIEW);
	if(!BOOLSETTING(SHOW_WINAMP_CONTROL)) PostMessage(WM_COMMAND, ID_TOGGLE_TOOLBAR);

        if (!WinUtil::isShift()) {
          PostMessage(WM_SPEAKER, AUTO_CONNECT);
        }

	PostMessage(WM_SPEAKER, PARSE_COMMAND_LINE);
	if (BOOLSETTING(OPEN_ADVICE)) {
		PostMessage(WM_SPEAKER, ACTIVATE_ADVICE_WINDOW);
	}

	try {
		File::ensureDirectory(SETTING(LOG_DIRECTORY));
	} catch (const FileException&) {	}

	startSocket();
#ifndef _DEBUG
        const int iconResourceId = IDR_MAINFRAME;
#else
        const int iconResourceId = IDI_MAGNET;
#endif
	m_normalicon.LoadIcon(iconResourceId, 16, 16);
	m_pmicon.LoadIcon(IDR_TRAY_PM, 16, 16);

	updateTray( BOOLSETTING( MINIMIZE_TRAY ) );

	Util::setAway(BOOLSETTING(AWAY));

	ctrlToolbar.CheckButton(IDC_AWAY,BOOLSETTING(AWAY));
        UISetCheck(IDC_AWAY, BOOLSETTING(AWAY));
	ctrlToolbar.CheckButton(IDC_LIMITER,BOOLSETTING(THROTTLE_ENABLE));
        UISetCheck(IDC_THROTTLE_ENABLE, BOOLSETTING(THROTTLE_ENABLE));
//	ctrlToolbar.CheckButton(ID_FILE_CONNECT,ConnectionManager::m_UseAutoBan); //[+]PPA
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, BOOLSETTING(SOUNDS_DISABLED));
	UISetCheck(IDC_ENABLE_SOUNDS, !BOOLSETTING(SOUNDS_DISABLED));
	ctrlToolbar.CheckButton(ID_TOGGLE_TOOLBAR, BOOLSETTING(SHOW_WINAMP_CONTROL));

#ifndef BETA
	if(SETTING(NICK).empty()) {
		// GENERATE NICK! Fix #PEERS1-58 && #PEERS1-43
		string nick = CID::generate().toBase32();
		SettingsManager::getInstance()->set(SettingsManager::NICK, nick);
	}
#endif

	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

HWND MainFrame::createWinampToolbar() {
	winampImages.CreateFromImage(IDB_WINAMP_CONTROL, 24, 24, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	winampImagesHot.CreateFromImage(IDB_WINAMP_CONTROL_HOT, 24, 24, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	
	ctrlWinampToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);
	ctrlWinampToolbar.SetImageList(winampImages);
	ctrlWinampToolbar.SetHotImageList(winampImagesHot);

	const int numButtons = 11;

	TBBUTTON tb[numButtons];
	memzero(tb, sizeof(tb));
	int n = 0, bitmap = 0;

#define ADD_BUTTON(a) \
	tb[n].iBitmap = bitmap++; \
	tb[n].idCommand = a; \
	tb[n].fsState = TBSTATE_ENABLED; \
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE; \
	n++;

#define ADD_SEPERATOR() \
	tb[n].fsStyle = TBSTYLE_SEP; \
	n++;

	//Fix buttons
	// back / play / pause / next / stop / volume up / volume 50% / volume down
	ADD_BUTTON(IDC_WINAMP_SPAM)
	ADD_SEPERATOR()
	ADD_BUTTON(IDC_WINAMP_BACK)
	ADD_BUTTON(IDC_WINAMP_PLAY)	
	ADD_BUTTON(IDC_WINAMP_PAUSE)
	ADD_BUTTON(IDC_WINAMP_NEXT)
	ADD_BUTTON(IDC_WINAMP_STOP)
	ADD_BUTTON(IDC_WINAMP_VOL_UP)
	ADD_BUTTON(IDC_WINAMP_VOL_HALF)
	ADD_BUTTON(IDC_WINAMP_VOL_DOWN)
	ADD_SEPERATOR()

	ctrlWinampToolbar.SetButtonStructSize();
	ctrlWinampToolbar.AddButtons(numButtons, tb);
	ctrlWinampToolbar.AutoSize();

	return ctrlWinampToolbar.m_hWnd;
}

LRESULT MainFrame::onWinampButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(SETTING(MEDIA_PLAYER) == 0) {
	HWND hwndWinamp = FindWindow(_T("Winamp v1.x"), NULL);
	if (::IsWindow(hwndWinamp)) {
		switch(wID) {
			case IDC_WINAMP_BACK: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON1, 0); break;
			case IDC_WINAMP_PLAY: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON2, 0); break;
			case IDC_WINAMP_STOP: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON4, 0); break;
			case IDC_WINAMP_PAUSE: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON3, 0); break;	
			case IDC_WINAMP_NEXT: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON5, 0); break;
			case IDC_WINAMP_VOL_UP: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_VOLUMEUP, 0); break;
			case IDC_WINAMP_VOL_DOWN: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_VOLUMEDOWN, 0); break;
            case IDC_WINAMP_VOL_HALF: SendMessage(hwndWinamp, WM_WA_IPC, 255/2, IPC_SETVOLUME); break;
		}
	}
	} else if(SETTING(MEDIA_PLAYER) == 1) {
		HWND hwndWMP = FindWindow(_T("WMPlayerApp"), NULL);
		if (::IsWindow(hwndWMP)) {
			switch(wID) {
				case IDC_WINAMP_BACK: SendMessage(hwndWMP, WM_COMMAND, WMP_PREV, 0); break;
				case IDC_WINAMP_PLAY: SendMessage(hwndWMP, WM_COMMAND, WMP_PLAY, 0); break;
				case IDC_WINAMP_STOP: SendMessage(hwndWMP, WM_COMMAND, WMP_STOP, 0); break;
				case IDC_WINAMP_PAUSE: SendMessage(hwndWMP, WM_COMMAND, WMP_PLAY, 0); break;	
				case IDC_WINAMP_NEXT: SendMessage(hwndWMP, WM_COMMAND, WMP_NEXT, 0); break;
				case IDC_WINAMP_VOL_UP: SendMessage(hwndWMP, WM_COMMAND, WMP_VOLUP, 0); break;
				case IDC_WINAMP_VOL_DOWN: SendMessage(hwndWMP, WM_COMMAND, WMP_VOLDOWN, 0); break;
				case IDC_WINAMP_VOL_HALF: SendMessage(hwndWMP, WM_COMMAND, WMP_MUTE, 0); break;
			}
		}
	} else if(SETTING(MEDIA_PLAYER) == 2) {
		// Since i couldn't find out the appropriate window messages, we doing this а la COM
		HWND hwndiTunes = FindWindow(_T("iTunes"), _T("iTunes"));
		if (::IsWindow(hwndiTunes)) {
			IiTunes *iITunes;
			CoInitialize(NULL);
			if (SUCCEEDED(::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&iITunes))) {
				long currVol;
				switch(wID) {
					case IDC_WINAMP_BACK: iITunes->PreviousTrack(); break;
					case IDC_WINAMP_PLAY: iITunes->Play(); break;
					case IDC_WINAMP_STOP: iITunes->Stop();  break;
					case IDC_WINAMP_PAUSE: iITunes->Pause(); break;	
					case IDC_WINAMP_NEXT: iITunes->NextTrack(); break;
					case IDC_WINAMP_VOL_UP: iITunes->get_SoundVolume(&currVol); iITunes->put_SoundVolume(currVol+10); break;
					case IDC_WINAMP_VOL_DOWN: iITunes->get_SoundVolume(&currVol); iITunes->put_SoundVolume(currVol-10); break;
					case IDC_WINAMP_VOL_HALF: iITunes->put_SoundVolume(50); break;
				}
			}
			iITunes->Release();
			CoUninitialize();
		}
	} else if(SETTING(MEDIA_PLAYER) == 3) {
		HWND hwndMPC = FindWindow(_T("MediaPlayerClassicW"), NULL);
		if (::IsWindow(hwndMPC)) {
			switch(wID) {
				case IDC_WINAMP_BACK: SendMessage(hwndMPC, WM_COMMAND, MPC_PREV, 0); break;
				case IDC_WINAMP_PLAY: SendMessage(hwndMPC, WM_COMMAND, MPC_PLAY, 0); break;
				case IDC_WINAMP_STOP: SendMessage(hwndMPC, WM_COMMAND, MPC_STOP, 0); break;
				case IDC_WINAMP_PAUSE: SendMessage(hwndMPC, WM_COMMAND, MPC_PAUSE, 0); break;	
				case IDC_WINAMP_NEXT: SendMessage(hwndMPC, WM_COMMAND, MPC_NEXT, 0); break;
				case IDC_WINAMP_VOL_UP: SendMessage(hwndMPC, WM_COMMAND, MPC_VOLUP, 0); break;
				case IDC_WINAMP_VOL_DOWN: SendMessage(hwndMPC, WM_COMMAND, MPC_VOLDOWN, 0); break;
				case IDC_WINAMP_VOL_HALF: SendMessage(hwndMPC, WM_COMMAND, MPC_MUTE, 0); break;
			}
		}
	}
       return 0;
}

void MainFrame::startSocket() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();

//	if(ClientManager::getInstance()->isActive()) {
		try {
			ConnectionManager::getInstance()->listen();
		} catch(const Exception&) {
#ifndef _DEBUG
			MessageBox(CTSTRING(TCP_PORT_BUSY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
#endif
		}
		try {
			SearchManager::getInstance()->listen();
		} catch(const Exception&) {
#ifndef _DEBUG
			MessageBox(CTSTRING(TCP_PORT_BUSY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
#endif
		}
//	}

#ifdef PPA_INCLUDE_UPNP
	startUPnP();
#endif

	detectNetworkSetup();
}

void MainFrame::detectNetworkSetup() {
	CWaitCursor wait;
	DetectNetworkSetupThread::run();
}

#ifdef PPA_INCLUDE_UPNP
void MainFrame::startUPnP() {
	stopUPnP();

	UPnP_TCPConnection = new UPnP( Util::getLocalIp(), "TCP", APPNAME " Download Port (" + Util::toString(ConnectionManager::getInstance()->getPort()) + " TCP)", ConnectionManager::getInstance()->getPort() );
	UPnP_UDPConnection = new UPnP( Util::getLocalIp(), "UDP", APPNAME " Search Port (" + Util::toString(SearchManager::getInstance()->getPort()) + " UDP)", SearchManager::getInstance()->getPort() );
		
	UPnP_UDPConnection->OpenPorts();
	UPnP_TCPConnection->OpenPorts();
}

void MainFrame::stopUPnP() {
	// Just check if the port mapping objects are initialized (NOT NULL)
	if ( UPnP_TCPConnection != NULL )
	{
		UPnP_TCPConnection->ClosePorts();
		delete UPnP_TCPConnection;
	}
	if ( UPnP_UDPConnection != NULL )
	{
		UPnP_UDPConnection->ClosePorts();
		delete UPnP_UDPConnection;
	}
	// Not sure this is required (i.e. Objects are checked later in execution)
	// But its better being on the save side :P
	UPnP_TCPConnection = UPnP_UDPConnection = NULL;
}

#endif

HWND MainFrame::createToolbar() {
	if(!tbarcreated) {
		ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);
                m_toolbarImages.init();
                ctrlToolbar.SetImageList(m_toolbarImages.getImages());
                ctrlToolbar.SetHotImageList(m_toolbarImages.getHotImages());		
		tbarcreated = true;
	}

	while(ctrlToolbar.GetButtonCount()>0)
		ctrlToolbar.DeleteButton(0);

	ctrlToolbar.SetButtonStructSize();
	StringTokenizer<string> t(SETTING(TOOLBAR), ',');
	StringList& l = t.getTokens();
	
	for(StringList::const_iterator k = l.begin(); k != l.end(); ++k) {
		int i = Util::toInt(*k);		
		
		TBBUTTON nTB;
		memzero(&nTB, sizeof(TBBUTTON));

		if((i == INT_MAX) || (i == -1)) {
			nTB.fsStyle = TBSTYLE_SEP;			
		} else {
			nTB.iBitmap = WinUtil::ToolbarButtons[i].image;
			nTB.idCommand = WinUtil::ToolbarButtons[i].id;
			nTB.fsState = TBSTATE_ENABLED;
			nTB.fsStyle = TBSTYLE_AUTOSIZE | ((WinUtil::ToolbarButtons[i].check == true)? TBSTYLE_CHECK : TBSTYLE_BUTTON);
			nTB.iString = WinUtil::ToolbarButtons[i].tooltip;
		}
		ctrlToolbar.AddButtons(1, &nTB);
	}	

	ctrlToolbar.AutoSize();

	return ctrlToolbar.m_hWnd;
}



LRESULT MainFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		
	if(wParam == DOWNLOAD_LISTING) {
		auto_ptr<DirectoryListInfo> i(reinterpret_cast<DirectoryListInfo*>(lParam));
		DirectoryListingFrame::openWindow(i->file, i->user, i->speed);
	} else if(wParam == BROWSE_LISTING) {
		auto_ptr<DirectoryBrowseInfo> i(reinterpret_cast<DirectoryBrowseInfo*>(lParam));
		DirectoryListingFrame::openWindow(i->user, i->text, 0);
	} else if(wParam == VIEW_FILE_AND_DELETE) {
		auto_ptr<tstring> file(reinterpret_cast<tstring*>(lParam));
                if (BOOLSETTING(EXTERNAL_PREVIEW)) // !SMT!-UI
                        ShellExecute(NULL, NULL, file->c_str(), NULL, NULL, SW_SHOW); // !SMT!-UI
                else {
		TextFrame::openWindow(*file);
		File::deleteFile(Text::fromT(*file));
                }
	} else if(wParam == STATS) {
		auto_ptr<TStringList> pstr(reinterpret_cast<TStringList*>(lParam));
                if (ctrlStatus.IsWindow()) {
                  CClientDC dc(ctrlStatus.m_hWnd);
                  bool u = false;
                  const TStringList& str = *pstr;
                  ctrlStatus.SetText(1, str[0].c_str());
                  for (int i = 1; i < 8; i++) {
                    int w = WinUtil::getTextWidth(str[i], dc);
                    if (statusSizes[i] < w) {
                      statusSizes[i] = w;
                      u = true;
                    }
                    ctrlStatus.SetText(i+1, str[i].c_str());
                  }
                  if (u) {
                    UpdateLayout(TRUE);
                  }
                }
		if (bShutdown) {
			uint64_t iSec = GET_TICK() / 1000;
			if (ctrlStatus.IsWindow()) {
				if(!isShutdownStatus) {
					ctrlStatus.SetIcon(9, hShutdownIcon);
					isShutdownStatus = true;
				}
				if (DownloadManager::getInstance()->getDownloadCount() > 0) {
					iCurrentShutdownTime = iSec;
					ctrlStatus.SetText(9, _T(""));
				} else {
					int64_t timeLeft = SETTING(SHUTDOWN_TIMEOUT) - (iSec - iCurrentShutdownTime);
					ctrlStatus.SetText(9, (_T(" ") + Util::formatSeconds(timeLeft, timeLeft < 3600)).c_str(), SBT_POPOUT);
					if (iCurrentShutdownTime + SETTING(SHUTDOWN_TIMEOUT) <= iSec) {
						bool bDidShutDown = false;
						bDidShutDown = WinUtil::shutDown(SETTING(SHUTDOWN_ACTION));
						if (bDidShutDown) {
							// Should we go faster here and force termination?
							// We "could" do a manual shutdown of this app...
						} else {
							ctrlStatus.SetText(0, CTSTRING(FAILED_TO_SHUTDOWN));
							ctrlStatus.SetText(9, _T(""));
						}
						// We better not try again. It WON'T work...
						bShutdown = false;
					}
				}
			}
		} else {
			if (ctrlStatus.IsWindow()) {
				if(isShutdownStatus) {
					ctrlStatus.SetText(9, _T(""));
					ctrlStatus.SetIcon(9, NULL);
					isShutdownStatus = false;
				}
			}
		}
	} else if(wParam == AUTO_CONNECT) {
		autoConnect(FavoriteManager::getInstance()->getFavoriteHubs());
	} else if(wParam == PARSE_COMMAND_LINE) {
		parseCommandLine(GetCommandLine());
	} else if(wParam == ACTIVATE_ADVICE_WINDOW) {
		class AdviceWindowActivator : public InvokeLater {
		public:
			AdviceWindowActivator(): InvokeLater(NULL,500), count(0) { }
		protected:
			int count;
			virtual bool executeAction() {
				AdviceFrame* frame = isAdviceWindowOpen();
				if (frame != NULL) {
					frame->MDIActivate(frame->m_hWnd);
					return ++count >= 4;
				}
				return false;
			}
		};
		AdviceWindowActivator* activator = new AdviceWindowActivator();
		activator->execute();
	} else if(wParam == STATUS_MESSAGE) {
		tstring* msg = (tstring*)lParam;
		if(ctrlStatus.IsWindow()) {
			tstring line = Text::toT("[" + Util::getShortTimeString() + "] ") + *msg;

			ctrlStatus.SetText(0, line.c_str());
			while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
				lastLinesList.erase(lastLinesList.begin());
			if (line.find(_T('\r')) == tstring::npos) {
				lastLinesList.push_back(line);
			} else {
				lastLinesList.push_back(line.substr(0, line.find(_T('\r'))));
			}
		}
		delete msg;
	} else if(wParam == SHOW_POPUP) {
		BallonPopup* msg = (BallonPopup*)lParam;
		PopupManager::getInstance()->Show(msg->Message, msg->Title, msg->Icon);
		delete msg;
	} else if(wParam == WM_CLOSE) {
    		PopupManager::getInstance()->Remove((int)lParam);
	} else if(wParam == SET_PM_TRAY_ICON) {
		if(bIsPM == false && (!WinUtil::isAppActive || bAppMinimized) && bTrayIcon == true) {
                        setIcon(m_pmicon);
			bIsPM = true;
		}
	} 
#ifdef PPA_INCLUDE_CHECK_UPDATE
        else if (wParam == UPDATE_RESTART_CONFIRM) {
          UpdateRestartConfirmDlg dlg;
          if (dlg.DoModal(m_hWnd) == IDOK) {
            if (AutoUpdate::execute()) {
              exitToUpdate = true;
              PostMessage(WM_CLOSE, 0, 0);
            }
          }
        }
#endif

	return 0;
}

void MainFrame::parseCommandLine(const tstring& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j));
		}
	if( (j = cmdLine.find(_T("adc://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j));
	}
	if( (j = cmdLine.find(_T("magnet:?"), i)) != string::npos) {
		WinUtil::parseMagnetUri(cmdLine.substr(j));
	}
}

static bool isVideo(const tstring &cmdLine) {
	string::size_type i;
	if ((i = cmdLine.find(_T("magnet:?"))) != string::npos) {
		StringTokenizer<tstring> mag(cmdLine.substr(i), _T('&'));
		for (TStringList::const_iterator idx = mag.getTokens().begin(); idx != mag.getTokens().end(); ++idx) {
			string::size_type pos = idx->find(_T('='));
			tstring name;
			if (pos != string::npos) {
				name = idx->substr(0, pos);
			} else {
				name = *idx;
			}
			if (name == _T("video")) {
				return true;
			}
		}
	}
	return false;
}

LRESULT MainFrame::onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
        if (!getPassword()) return false; // !SMT!-f
        tstring cmdLine = (LPCTSTR) (((COPYDATASTRUCT *)lParam)->lpData);
        if (!isVideo(cmdLine) && IsIconic()) {
                ShowWindow(SW_RESTORE);
        }
        parseCommandLine(Text::toT(WinUtil::getAppName() + " ") + cmdLine);
        return true;
}

LRESULT MainFrame::onHashProgress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HashProgressDlg(false).DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::onOpenWindows(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID) {
		case ID_FILE_SEARCH: m_searchBar.setFocusToSearch(); break;
		case IDC_FAVORITES: FavoriteHubsFrame::openWindow(); break;
		case IDC_FAVUSERS: UsersFrame::openWindow(); break;
		case IDC_NOTEPAD: NotepadFrame::openWindow(); break;
		case IDC_QUEUE: QueueFrame::openWindow(); break;
		case IDC_SEARCH_SPY: SpyFrame::openWindow(); break;
		case IDC_FILE_ADL_SEARCH: ADLSearchFrame::openWindow(); break;
		case IDC_NET_STATS: StatsFrame::openWindow(); break; 
		case IDC_FINISHED: FinishedFrame::openWindow(); break;
		case IDC_FINISHED_UL: FinishedULFrame::openWindow(); break;
		case IDC_UPLOAD_QUEUE: WaitingUsersFrame::openWindow(); break;
		case IDC_CDMDEBUG_WINDOW: CDMDebugFrame::openWindow(); break;
		case IDC_ADVICE_WINDOW: AdviceFrame::openWindow(false); break;
		case IDC_RECENTS: RecentHubsFrame::openWindow(); break;
		default: dcassert(0); break;
	}
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  PropertiesDlg dlg(m_hWnd, SettingsManager::getInstance());
  const unsigned short lastTCP = static_cast<unsigned short>(SETTING(TCP_PORT));
  const unsigned short lastUDP = static_cast<unsigned short>(SETTING(UDP_PORT));
#ifdef PPA_INCLUDE_SSL
  const unsigned short lastTLS = static_cast<unsigned short>(SETTING(TLS_PORT));
#endif
  const int lastConn = SETTING(INCOMING_CONNECTIONS);
  const bool lastSortFavUsersFirst = BOOLSETTING(SORT_FAVUSERS_FIRST);
  if (dlg.DoModal(m_hWnd) == IDOK) {
    SettingsManager::getInstance()->save();
    if (missedAutoConnect && !SETTING(NICK).empty()) {
      PostMessage(WM_SPEAKER, AUTO_CONNECT);
    }
    if (SETTING(INCOMING_CONNECTIONS) != lastConn 
      || SETTING(TCP_PORT) != lastTCP
      || SETTING(UDP_PORT) != lastUDP 
#ifdef PPA_INCLUDE_SSL
      || SETTING(TLS_PORT) != lastTLS
#endif
      ) {
        startSocket();
    }
    ClientManager::getInstance()->infoUpdated(false);
    if (BOOLSETTING(SORT_FAVUSERS_FIRST) != lastSortFavUsersFirst) {
      HubFrameFactory::resortUsers();
    }
    if (BOOLSETTING(URL_HANDLER)) {
      WinUtil::registerDchubHandler();
      WinUtil::registerADChubHandler();
      WinUtil::urlDcADCRegistered = true;
    }
    else if (WinUtil::urlDcADCRegistered) {
      WinUtil::unRegisterDchubHandler();
      WinUtil::unRegisterADChubHandler();
      WinUtil::urlDcADCRegistered = false;
    }
    if (BOOLSETTING(MAGNET_REGISTER)) {
      WinUtil::registerMagnetHandler();
      WinUtil::urlMagnetRegistered = true; 
    }
    else if(WinUtil::urlMagnetRegistered) {
      WinUtil::unRegisterMagnetHandler();
      WinUtil::urlMagnetRegistered = false;
    }
    MainFrame::setLimiterButton(BOOLSETTING(THROTTLE_ENABLE));
    ctrlToolbar.CheckButton(IDC_AWAY, Util::getAway());
    ctrlToolbar.CheckButton(IDC_SHUTDOWN, getShutDown());
    updateTray(BOOLSETTING(MINIMIZE_TRAY));
  }
  else if (SETTING(NICK).empty()) {
    exitToUpdate = true;
    PostMessage(WM_CLOSE);
  }
  return 0;
}

class SharedFilesPropertiesDlg : public PropertiesDlg {
public:
  SharedFilesPropertiesDlg (HWND parent, SettingsManager* s): PropertiesDlg(parent, s) { }
protected:
  virtual void initPages() {
    pages.push_back(new UploadPage());
  }
};

LRESULT MainFrame::onFileListAddFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  SharedFilesPropertiesDlg dlg(m_hWnd, SettingsManager::getInstance());
  if (dlg.DoModal(m_hWnd) == IDOK) {
    SettingsManager::getInstance()->save();
  }
  return 0;
}

#ifdef PPA_INCLUDE_CHECK_UPDATE
void MainFrame::on(HttpConnectionListener::Complete, HttpConnection* /*aConn*/, const string&) throw() {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
		xml.stepIn();
#ifdef _DEBUG
		debugTrace("Check update: url=%s\n", xml.getChildData("URL").c_str());
#endif
		if (!isMainUpdate(xml)) {
			xml.resetCurrentChild();
			while (xml.findChild("file")) {
				const string name = xml.getChildAttrib("name");
				dcdebug("<file name='%s'> found\n", name.c_str());
				if (name == "configuration.xml") {
					xml.stepIn();
					const size_t inputSize = Util::toInt(xml.getChildData("Size"));
					const string inputMD5 = xml.getChildData("MD5");
					pair<int64_t,string> info = ConfigurationPatcher::getFileInfo();
					if (info.first != inputSize || Util::stricmp(info.second, inputMD5) != 0) {
						const string url = xml.getChildData("URL");
						LOG_MESSAGE("Start downloading " + url);
						downloader.reset(new HttpUpdateDownloader(false, UPDATE_MODE_CONFIGURATION, ConfigurationPatcher::getFilename(), inputSize, inputMD5));
						downloader->setListener(this);
						downloader->downloadFile(url);
						break;
					}
					xml.stepOut();
				}
#ifdef INCLUDE_PROVIDE_SELECTION
				else if (name == "ip-blocks.xml") {
					xml.stepIn();
					const size_t inputSize = Util::toInt(xml.getChildData("Size"));
					const string inputMD5 = xml.getChildData("MD5");
					pair<int64_t,string> info = IpBlocksLoader::getIpBlocksInfo();
					if (info.first != inputSize || Util::stricmp(info.second, inputMD5) != 0) {
						const string url = xml.getChildData("URL");
						LOG_MESSAGE("Start downloading " + url);
						downloader.reset(new HttpUpdateDownloader(false, UPDATE_MODE_IP_BLOCKS, IpBlocksLoader::getIpBlocksFilename(), inputSize, inputMD5));
						downloader->setListener(this);
						downloader->downloadFile(url);
						break;
					}
					xml.stepOut();
				}
#endif
			}
		} 
	}
	catch (const Exception& e) {
		LOG_MESSAGE("Error parsing update check response: " + e.getError());
	}
}

// returns true if new download started
bool MainFrame::isMainUpdate(SimpleXML& xml) {
	string buildNumber = xml.getChildData("Build");
	if (!buildNumber.empty()) {
		if (Util::toDouble(buildNumber) > BUILDID) {
			LOG_MESSAGE("Version " + buildNumber + " available");
			string url = xml.getChildData("URL");
			//TODO select output file name with the same name as in url
			tstring updatePath = AutoUpdate::getUpdateTargetPath();
			string md5 = xml.getChildData("MD5");
			size_t size = Util::toInt(xml.getChildData("Size"));
			if (!md5.empty() && size > 0) {
				if (AutoUpdate::checkFile(updatePath, size, md5)) {
					/* файл существует, размер и md5 совпадают - выходим */
					return false;
				}
			}
			LOG_MESSAGE("Start downloading version " + buildNumber + " from " + url);
			downloader.reset(new HttpUpdateDownloader(true, UPDATE_MODE_APP, updatePath, size, md5));
			downloader->setListener(this);
			downloader->downloadFile(url);
			return true;
		}
	}
	return false;
}

void MainFrame::onDownloadComplete(HttpUpdateDownloader* downloader) throw() {
	switch (downloader->getMode())
	{
	case UPDATE_MODE_APP:
		PostMessage(WM_SPEAKER, UPDATE_RESTART_CONFIRM, 0);
		break;
#ifdef INCLUDE_PROVIDE_SELECTION
	case UPDATE_MODE_IP_BLOCKS:
		{
			const int provider = SETTING(PROVIDER);
			if (provider != 0 && provider != PGLoader::UNLIM_ANY_PROVIDER) {
				PGLoader::getInstance()->LoadIPFilters();
			}
		}
		break;
#endif
	case UPDATE_MODE_CONFIGURATION:
		ConfigurationPatcher::load();
		KillTimer(PINGER_TIMER);

		if (!SETTING(HTTP_PING_ADDRESS).empty()) {
			pinger->downloadFile(SETTING(HTTP_PING_ADDRESS));
			if (SETTING(HTTP_PING_INTERVAL) > 60*1000) {
				SetTimer(PINGER_TIMER, SETTING(HTTP_PING_INTERVAL));
			}
		}

		break;
	}
}

LRESULT MainFrame::onTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (wParam == TIMER_CHECK_UPDATE) {
    StringMap params;
    params["message"] = "Periodic updates check";
    LOG(LogManager::SYSTEM, params);
    versionInfo = Util::emptyString;
    updatesChecker->downloadFile(string(VERSIONFILE) + "?V=" + Util::toString(BUILDID) + "&M=P&NICK=" + SETTING(NICK) + "&CID=" + SETTING(PRIVATE_ID));
  } else if (wParam == PINGER_TIMER) {
	  if (!SETTING(HTTP_PING_ADDRESS).empty()) {
		pinger->downloadFile(SETTING(HTTP_PING_ADDRESS));
	  }
  }
  return 0;
}

#endif

LRESULT MainFrame::onWebServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
 	WebServerManager::getInstance()->getServerSocket().incoming();
 	return 0;
}

LRESULT MainFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;

	if((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
	{
		int stringId = -1;
            switch(idCtrl) {
  	                         case IDC_WINAMP_BACK: stringId = ResourceManager::WINAMP_BACK; break;
  	                         case IDC_WINAMP_PLAY: stringId = ResourceManager::WINAMP_PLAY; break;
  	                         case IDC_WINAMP_PAUSE: stringId = ResourceManager::WINAMP_PAUSE; break;
  	                         case IDC_WINAMP_NEXT: stringId = ResourceManager::WINAMP_NEXT; break;
  	                         case IDC_WINAMP_STOP: stringId = ResourceManager::WINAMP_STOP; break;
  	                         case IDC_WINAMP_VOL_UP: stringId = ResourceManager::WINAMP_VOL_UP; break;
  	                         case IDC_WINAMP_VOL_HALF: stringId = ResourceManager::WINAMP_VOL_HALF; break;
  	                         case IDC_WINAMP_VOL_DOWN: stringId = ResourceManager::WINAMP_VOL_DOWN; break;
  	                         case IDC_WINAMP_SPAM: stringId = ResourceManager::WINAMP_SPAM; break;
  	                 }

		for(int i = 0; WinUtil::ToolbarButtons[i].id != 0; i++) {
			if(WinUtil::ToolbarButtons[i].id == idCtrl) {
				stringId = WinUtil::ToolbarButtons[i].tooltip;
				break;
			}
		}
		if(stringId != -1) {
			_tcsncpy(pDispInfo->lpszText, CTSTRING_I((ResourceManager::Strings)stringId), 79);
			pDispInfo->uFlags |= TTF_DI_SETITEM;
		}
	} else { // if we're really in the status bar, this should be detected intelligently
		lastLines.clear();
		for(TStringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
			lastLines += *i;
			lastLines += _T("\r\n");
		}
		if(lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}
		pDispInfo->lpszText = const_cast<TCHAR*>(lastLines.c_str());
	}
	return 0;
}

void MainFrame::autoConnect(const FavoriteHubEntry::List& fl) {
 	missedAutoConnect = false;
	FavoriteHubEntry::List entries(fl);
	for (size_t i = 1; i < entries.size(); ++i) {
		if (Util::findSubString(entries[i]->getServer(), PeersUtils::PEERS_HUB) != string::npos) {
			entries.insert(entries.begin(), entries[i]);
			entries.erase(&entries.at(i + 1));
			break;
		}
	}
	for(FavoriteHubEntry::List::const_iterator i = entries.begin(); i != entries.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		if(entry->getConnect()) {
 			if(!entry->getNick().empty() || !SETTING(NICK).empty()) {
				RecentHubEntry r;
				r.setName(entry->getName());
				r.setDescription(entry->getDescription());
				r.setUsers("*");
				r.setShared("*");
				r.setServer(entry->getServer());
				FavoriteManager::getInstance()->addRecent(r);
				HubFrameFactory::openWindow(Text::toT(entry->getServer())
					, true
					, Text::toT(entry->getRawOne())
					, Text::toT(entry->getRawTwo())
					, Text::toT(entry->getRawThree())
					, Text::toT(entry->getRawFour())
					, Text::toT(entry->getRawFive())
					, entry->getWindowPosX(), entry->getWindowPosY(), entry->getWindowSizeX(), entry->getWindowSizeY(), entry->getWindowType(), entry->getChatUserSplit(), entry->getUserListState());
 			} else
 				missedAutoConnect = true;
 		}				
	}
	class HubActivator : public InvokeLater {
	public:
		HubActivator(): InvokeLater(NULL,500) { }
	protected:
		virtual bool executeAction() {
			return HubFrameFactory::activateHubWindow(PeersUtils::PEERS_HUB, BOOLSETTING(OPEN_HUB_CHAT_ON_CONNECT));
		}
	};
	if (!isAdviceWindowOpen()) {
		HubActivator* hubActivator = new HubActivator();
		hubActivator->execute();
	}
}

void MainFrame::updateTray(bool add /* = true */) {
  if (g_RunningUnderWine) {
    return; 	//[+]PPA  
  }
  if (add) {
    if (!bTrayIcon) {
		NOTIFYICONDATA nid = {0};
      nid.cbSize = sizeof(NOTIFYICONDATA);
      nid.hWnd = m_hWnd;
      nid.uID = 0;
      nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
      nid.uCallbackMessage = WPM_TRAY;
      nid.hIcon = m_normalicon;
      _tcsncpy(nid.szTip, _T(APPNAME), 64);
      nid.szTip[63] = '\0';
      lastMove = GET_TICK() - 1000;
      ::Shell_NotifyIcon(NIM_ADD, &nid);
      bTrayIcon = true;
    }
  } 
  else {
    if (bTrayIcon) {
		NOTIFYICONDATA nid={0};
      nid.cbSize = sizeof(NOTIFYICONDATA);
      nid.hWnd = m_hWnd;
      nid.uID = 0;
      nid.uFlags = 0;
      ::Shell_NotifyIcon(NIM_DELETE, &nid);
      ShowWindow(SW_SHOW);
      bTrayIcon = false;
    }
  }
}

LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
  if (wParam == SIZE_MINIMIZED) {
    if (IsWindowVisible()) {
      dcdebug("MainFrame::onSize - SIZE_MINIMIZED\n");
      /* почему-то про первую минимизацию после запуска приходило два сообщения */
      SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
      if (BOOLSETTING(AUTO_AWAY)) {
        if (!bAppMinimized) {
          if (Util::getAway()) {
            awaybyminimize = false;
          } 
          else {
            awaybyminimize = true;
            Util::setAway(true);
            setAwayButton(true);
            ClientManager::getInstance()->infoUpdated(false);
          }
        }
      }
      bAppMinimized = true;
	  Util::setMinimized(bAppMinimized);
      if (!g_RunningUnderWine) {
        if (BOOLSETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
          updateTray(true);
          ShowWindow(SW_HIDE);
          m_OLDPriorityClass = GetPriorityClass(GetCurrentProcess());
          SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
        } 
        else {
          updateTray(false);
        }
      }
      maximized = IsZoomed() > 0;
    }
    vector<MDIContainer::Window> windows = MDIContainer::list();
    for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
      (*i)->SendMessage(WPM_APP_MINIMIZE);
    }
  } 
  else if ((wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)) {
    if (m_OLDPriorityClass) {
      SetPriorityClass(GetCurrentProcess(), m_OLDPriorityClass);
    }
    if (BOOLSETTING(AUTO_AWAY)) {
      if (awaybyminimize) {
        awaybyminimize = false;
        Util::setAway(false);
        setAwayButton(false);
        ClientManager::getInstance()->infoUpdated(false);
      }
    }
    if (bIsPM && bTrayIcon) {
      setIcon(m_normalicon);
      bIsPM = false;
    }
	bAppMinimized = false;
	Util::setMinimized(bAppMinimized);
  }
  bHandled = FALSE;
  return 0;
}

LRESULT MainFrame::onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef PPA_INCLUDE_CHECK_UPDATE
	KillTimer(TIMER_CHECK_UPDATE);
	if(updatesChecker != NULL) {
		updatesChecker->removeListener(this);
		delete updatesChecker;
		updatesChecker = NULL;
	}
	downloader.reset();
#endif
	KillTimer(PINGER_TIMER);
	if (pinger != NULL) {
		delete pinger;
		pinger = NULL;
	}

	saveWindowPlacement();

	QueueManager::getInstance()->saveQueue();
	SettingsManager::getInstance()->save();
	
	return 0;
}

void MainFrame::saveWindowPlacement() {
	if (bAppMinimized) {
		// do not save location if window is minimized
		return;
	}
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);

	if (BOOLSETTING(SHOW_TRANSFERVIEW)) {
		SettingsManager::getInstance()->set(SettingsManager::TRANSFER_SPLIT_SIZE, m_nProportionalPos);
	}
	if (wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
		CRect rc;
		GetWindowRect(rc);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
	}
	if (wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE) {
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);
	}
}

static SplashWindow* shutdownSplash = NULL;

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closing) {
		if (BOOLSETTING(MINIMIZE_ON_CLOSE) && !exitToUpdate && !closeEnabled) {
			ShowWindow(SW_MINIMIZE);
			return 0;
		}
		bool allowClose = true;
		if (!exitToUpdate && SETTING(PROTECT_CLOSE) && hasPassword()) {
			LineDlg dlg;
			dlg.description = TSTRING(PASSWORD_DESC);
			dlg.title = TSTRING(PASSWORD_TITLE);
			dlg.password = true;
			dlg.disable = true;
			if (dlg.DoModal(m_hWnd) == IDOK) {
				tstring tmp = dlg.line;
				TigerTree mytth(TigerTree::calcBlockSize(tmp.size(), 1));
				mytth.update(tmp.c_str(), tmp.size());
				mytth.finalize();
				allowClose = mytth.getRoot().toBase32().c_str() == SETTING(PASSWORD);
			}
			else {
				allowClose = false;
			}
		}
		if (allowClose && (exitToUpdate || !BOOLSETTING(CONFIRM_EXIT) || MessageBox(CTSTRING(REALLY_EXIT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)) {
			exitToUpdate = false;
#ifdef PPA_INCLUDE_CHECK_UPDATE
			KillTimer(TIMER_CHECK_UPDATE);
			if (updatesChecker != NULL) {
				updatesChecker->removeListener(this);
				delete updatesChecker;
				updatesChecker = NULL;
			}
			downloader.reset();
#endif
			KillTimer(PINGER_TIMER);
			if (pinger != NULL) {
				delete pinger;
				pinger = NULL;
			}
#ifndef _DEBUG
			shutdownSplash = new SplashWindow(IDB_SPLASH);
			shutdownSplash->init();
			shutdownSplash->setSplashText(_T("Завершение работы..."));
#endif
			CReBarCtrl rebar = m_hWndToolBar;
			ToolbarManager::getInstance()->getFrom(rebar, "MainToolBar");

			updateTray(false);

			saveWindowPlacement();

			ShowWindow(SW_HIDE);
			transferView.prepareClose();

			WinUtil::stopVideo();

			WebServerManager::getInstance()->removeListener(this);
			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();
			listQueue.shutdown();

#ifdef PPA_INCLUDE_UPNP
			stopUPnP();
#endif
			DWORD id;
			stopperThread = CreateThread(NULL, 0, stopper, (MainFrame*)this, 0, &id);
			closing = true;
		}
		else {
			if (closeEnabled) {
				closeEnabled = false;
			}
		}
		bHandled = TRUE;
	} 
	else {
		// This should end immediately, as it only should be the stopper that sends another WM_CLOSE
		WaitForSingleObject(stopperThread, 60*1000);
		CloseHandle(stopperThread);
		stopperThread = NULL;
		m_normalicon.DestroyIcon();
		DestroyIcon(hShutdownIcon); 	
		m_pmicon.DestroyIcon();
		bHandled = FALSE;
	}
	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID) {
	case IDC_HELP_HOMEPAGE: WinUtil::openLink(HOMEPAGE); break;
	case IDC_HELP_DISCUSS: WinUtil::openLink(_T(DISCUSS)); break;
//[-]PPA	case IDC_HELP_GEOIPFILE: site = _T(GEOIPFILE); break;
	case IDC_HELP_DONATE: WinUtil::openLink(HOMEPAGE); break;
//[-]PPA	case IDC_GUIDES: site = _T(GUIDES); break;
//[-]PPA        case IDC_GUIDE: site = _T(GUIDE); break;
//[-]PPA	case IDC_SITES_TAN: site = _T(SITES_TAN); break;
    case IDC_SITES_FLYLINK_TRAC: WinUtil::openLink(_T(SITES_FLYLINK_TRAC)); break;
//[-]PPA	case IDC_SITES_NXP: site = _T(SITES_NXP); break;
	default: dcassert(0);
	}
	return 0;
}

int MainFrame::run() {
	tstring file;
	if(WinUtil::browseFile(file, m_hWnd, false, lastTTHdir) == IDOK) {
		WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_GRAYED);
		Thread::setThreadPriority(Thread::LOW);
		lastTTHdir = Util::getFilePath(file);

		char TTH[192*8/(5*8)+2];
		char buf[512*1024];

		try {
			File f(Text::fromT(file), File::READ, File::OPEN);
			TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));

			if(f.getSize() > 0) {
				size_t n = 512*1024;
				while( (n = f.read(buf, n)) > 0) {
					tth.update(buf, n);
					n = 512*1024;
				}
			} else {
				tth.update("", 0);
			}
			tth.finalize();

			strcpy(TTH, tth.getRoot().toBase32().c_str());

			CInputBox ibox(m_hWnd);

			string magnetlink = "magnet:?xt=urn:tree:tiger:"+ string(TTH) +"&xl="+Util::toString(f.getSize())+"&dn="+Util::encodeURI(Text::fromT(Util::getFileName(file)));
			f.close();
			
			ibox.DoModal(_T("Tiger Tree Hash"), file.c_str(), Text::toT(TTH).c_str(), Text::toT(magnetlink).c_str());
		} catch(...) { }
		Thread::setThreadPriority(Thread::NORMAL);
	WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_ENABLED);
	}
	return 0;
}

LRESULT MainFrame::onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Thread::start();
	return 0;
}

void MainFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
  RECT rect;
  GetClientRect(&rect);
  // position bars and offset their dimensions
  UpdateBarsPosition(rect, bResizeBars);

  if (ctrlStatus.IsWindow() && ctrlLastLines.IsWindow()) {
    CRect sr;
    int w[10];
    ctrlStatus.GetClientRect(sr);
    w[9] = sr.right - 20;
    w[8] = w[9] - 60;
    for (int x = 7; x>=0; --x) {
      w[x] = max(w[x+1] - statusSizes[x], 0);
    }
    ctrlStatus.SetParts(10, w);
    ctrlLastLines.SetMaxTipWidth(w[0]);
  }
  if (m_peersToolbar) {
    CRect r(rect);
    r.bottom = r.top + m_peersToolbar.getHeight();
    m_peersToolbar.MoveWindow(r);
    rect.top = r.bottom;
  }
  if (m_searchBar) {
	  CRect r(rect);
	  r.bottom = r.top + m_searchBar.getHeight();
	  m_searchBar.MoveWindow(r);
	  rect.top = r.bottom;
  }
  if (ctrlTab.IsWindow()) {
    CRect rc(rect);
    rc.bottom = rc.top + ctrlTab.getHeight();
    ctrlTab.MoveWindow(rc);
    rect.top = rc.bottom;
  }
#if 0
  if (m_hubMessages.IsWindow()) {
    CRect r(rect);
    r.top = r.bottom - m_hubMessages.getPreferredHeight();
    m_hubMessages.MoveWindow(r);
    rect.bottom = r.top;
  }
#endif
  SetSplitterRect(&rect);
}

int MainFrame::getMinChildHeight() const {
  vector<MDIContainer::Window> windows = MDIContainer::list();
  int minHeight = 0;
  for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
    MDIContainer::Window window = *i;
    ChildMinMaxInfo minMaxInfo;
    window->SendMessage(WPM_MDI_CHILD_GETMINMAXINFO, 0, (LPARAM) &minMaxInfo);
    if (minMaxInfo.m_minHeight > minHeight) {
      minHeight = minMaxInfo.m_minHeight;
    }
  }
  return minHeight;
}

bool MainFrame::SetSplitterPos(int xyPos, bool bUpdate)
{
	splitterResizeActive = true;
	const int minChildHeight = max(64, getMinChildHeight());
	if (xyPos < minChildHeight) {
		xyPos = minChildHeight;
	}
	const int cxyMax = m_rcSplitter.bottom - m_rcSplitter.top;
	const int cxyRange = cxyMax - m_cxySplitBar - m_cxyBarEdge - transferView.getHeaderHeight() - m_hubMessages.getPreferredHeight() - m_footer.m_cxySplitBar - m_footer.m_cxyBarEdge;
	if (xyPos > cxyRange) {
		xyPos = cxyRange;
	}
	else if (!m_footer.splitterResizeActive) {
		const int size = cxyRange - xyPos;
		if (size < transferView.getHeaderHeight()) {
			xyPos = cxyRange;
		}
	}
	bool result = splitterBase::SetSplitterPos(xyPos, bUpdate);
	if (transferView.isVisible()) {
		transferView.setProportionalPos(m_nProportionalPos);
	}
	splitterResizeActive = false;
	return result;
}

static const TCHAR types[] = _T("File Lists\0*.xml.bz2\0All Files\0*.*\0");

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (wID == IDC_OPEN_MY_LIST){
    const string ownFileList = ShareManager::getInstance()->getOwnListFile();
    if (!ownFileList.empty()){
      DirectoryListingFrame::openWindow(Text::toT(ownFileList), ClientManager::getInstance()->getMe(), 0);
    }
  }
  else {
    tstring file;
    if (WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), types)) {
      UserPtr u = DirectoryListing::getUserFromFilename(Text::fromT(file));
      if (u) {
        DirectoryListingFrame::openWindow(file, u, 0);
      }
      else {
        MessageBox(CTSTRING(INVALID_LISTNAME), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
      }
    }
  }
  return 0;
}

LRESULT MainFrame::onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true);
	return 0;
}

// !SMT!-f
bool MainFrame::getPassword()
{
  if (!SETTING(PROTECT_TRAY) || maximized || !hasPassword()) {
    return true;
  }
  LineDlg dlg;
  dlg.description = TSTRING(PASSWORD_DESC);
  dlg.title = TSTRING(PASSWORD_TITLE);
  dlg.password = true;
  dlg.disable = true;
  if (dlg.DoModal(m_hWnd) != IDOK) {
    return false;
  }
  tstring tmp = dlg.line;
  TigerTree mytth(TigerTree::calcBlockSize(tmp.size(), 1));
  mytth.update(tmp.c_str(), tmp.size());
  mytth.finalize();
  return (Text::toT(mytth.getRoot().toBase32().c_str()) == Text::toT(SETTING(PASSWORD)));
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  if (lParam == WM_LBUTTONUP) {
    if (bAppMinimized) {
      if (getPassword()) {
        ShowWindow(SW_SHOW);
        ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
      }
    }
    else {
      ShowWindow(SW_MINIMIZE);
    }
  }
  else if (lParam == WM_MOUSEMOVE && ((lastMove + 1000) < GET_TICK()) ) {
	  NOTIFYICONDATA nid={0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hWnd;
    nid.uID = 0;
    nid.uFlags = NIF_TIP;
    DownloadManager* dm = DownloadManager::getInstance();
    _tcsncpy(nid.szTip, (_T("D: ") + Util::formatBytesW(dm->getRunningAverage()) + _T("/s (") + 
      Util::toStringW(dm->getDownloadCount()) + _T(")\r\nU: ") +
      Util::formatBytesW(UploadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
      Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T(")")
      + _T("\r\nUptime: ") + Util::formatSeconds(Util::getUptime())
      ).c_str(), 63);

    ::Shell_NotifyIcon(NIM_MODIFY, &nid);
    lastMove = GET_TICK();
  }
  else if (lParam == WM_RBUTTONUP) {
    CPoint pt(GetMessagePos());		
    SetForegroundWindow(m_hWnd);
    if((!SETTING(PROTECT_TRAY) || !bAppMinimized) || !hasPassword())
      trayMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);		
    PostMessage(WM_NULL, 0, 0);
  }
  return 0;
}

LRESULT MainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewWinampBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 2);	// toolbar is 3nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_TOGGLE_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_WINAMP_CONTROL, bVisible);
	ctrlToolbar.CheckButton(ID_TOGGLE_TOOLBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewTopmost(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UISetCheck(IDC_TOPMOST, !BOOLSETTING(TOPMOST));
	SettingsManager::getInstance()->set(SettingsManager::TOPMOST, !BOOLSETTING(TOPMOST));
	toggleTopmost();
	return 0;
}

LRESULT MainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	bool bVisible = !transferView.isVisible();
        transferView.setVisibility(bVisible);
	UpdateLayout();
	return 0;
}

LRESULT MainFrame::onCloseWindows(WORD , WORD wID, HWND , BOOL& ) {
  switch(wID)
  {
  case IDC_CLOSE_DISCONNECTED:		HubFrameFactory::closeDisconnected();		break;
  case IDC_CLOSE_ALL_HUBS:		HubFrameFactory::closeAll();				break;
  case IDC_CLOSE_HUBS_BELOW:		HubFrameFactory::closeAll(SETTING(USER_THERSHOLD)); break;
  case IDC_CLOSE_HUBS_NO_USR:		HubFrameFactory::closeAll(2);				break;
  case IDC_CLOSE_ALL_PM:		PrivateFrameFactory::closeAll();			break;
  case IDC_CLOSE_ALL_OFFLINE_PM:	PrivateFrameFactory::closeAllOffline();	break;
  case IDC_CLOSE_ALL_DIR_LIST:		DirectoryListingFrame::closeAll();	break;
  case IDC_CLOSE_ALL_SEARCH_FRAME:	SearchFrameFactory::closeAll();			break;
  case IDC_RECONNECT_DISCONNECTED:	HubFrameFactory::reconnectDisconnected();	break;
  }
  return 0;
}

LRESULT MainFrame::onLimiter(WORD , WORD , HWND, BOOL& ) {
  const bool value = !BOOLSETTING(THROTTLE_ENABLE);
  SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE, value);
  setLimiterButton(value);
  ClientManager::getInstance()->infoUpdated(true);
  return 0;
}

LRESULT MainFrame::onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	LineDlg dlg;
	dlg.description = TSTRING(HUB_ADDRESS);
	dlg.title = TSTRING(QUICK_CONNECT);
	if(dlg.DoModal(m_hWnd) == IDOK){
		if(SETTING(NICK).empty()) {
			MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
			return 0;
		}

		tstring tmp = dlg.line;
		// Strip out all the spaces
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		if(!tmp.empty()) {
			RecentHubEntry r;
			r.setName("*");
			r.setDescription("*");
			r.setUsers("*");
			r.setShared("*");
			r.setServer(Text::fromT(tmp));
			FavoriteManager::getInstance()->addRecent(r);
			HubFrameFactory::openWindow(tmp,false);
		}
	}
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, uint32_t aTick) throw() {

	if (bIsPM && bTrayIcon == true) { // !SMT!-UI
		setIcon(((aTick/1000) & 1)? m_normalicon : m_pmicon); // !SMT!-UI
	}

		Util::increaseUptime();
		int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
		int64_t updiff = Socket::getTotalUp() - lastUp;
		int64_t downdiff = Socket::getTotalDown() - lastDown;

		TStringList* str = new TStringList();
		str->push_back(Util::getAway() ? TSTRING(AWAY) : _T(""));
		str->push_back(TSTRING(SHARED) + _T(": ") + Util::formatBytesW(ShareManager::getInstance()->getSharedSize()));
		str->push_back(_T("H: ") + Text::toT(Client::getCounts()));
		str->push_back(TSTRING(SLOTS) + _T(": ") + Util::toStringW(UploadManager::getInstance()->getFreeSlots()) + _T('/') + Util::toStringW(UploadManager::getInstance()->getSlots()) + _T(" (") + Util::toStringW(UploadManager::getInstance()->getFreeExtraSlots()) + _T('/') + Util::toStringW(SETTING(EXTRA_SLOTS)) + _T(")"));
		str->push_back(_T("D: ") + Util::formatBytesW(Socket::getTotalDown()));
		str->push_back(_T("U: ") + Util::formatBytesW(Socket::getTotalUp()));
		str->push_back(_T("D: [") + Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T("][") + (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) == 0 ? (tstring)_T("N") : Util::toStringW((int)SETTING(MAX_DOWNLOAD_SPEED_LIMIT)) + _T("k")) + _T("] ") + Util::formatBytesW(downdiff*1000I64/diff) + _T("/s"));
		str->push_back(_T("U: [") + Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("][") + (SETTING(MAX_UPLOAD_SPEED_LIMIT) == 0 ? (tstring)_T("N") : Util::toStringW((int)SETTING(MAX_UPLOAD_SPEED_LIMIT)) + _T("k")) + _T("] ") + Util::formatBytesW(updiff*1000I64/diff) + _T("/s"));
		PostMessage(WM_SPEAKER, STATS, (LPARAM)str);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
		lastUpdate = aTick;
		lastUp = Socket::getTotalUp();
		lastDown = Socket::getTotalDown();

                if(BOOLSETTING(THROTTLE_ENABLE)) {
			// Limitery sem a tam, vsude kam se podivam :o)
/* !SMT!-S
                        if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) > 0) {
                                if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) < ((5 * UploadManager::getInstance()->getSlots()) + 4) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_NORMAL, ((5 * UploadManager::getInstance()->getSlots()) + 4) );
				}
                                if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) == 0) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, (SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL)*7) );
				}
			}

                        if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) > 0) {
                                if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < ((5 * UploadManager::getInstance()->getSlots()) + 4) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, ((5 * UploadManager::getInstance()->getSlots()) + 4) );
				}
                                if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0) ) {
					SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, (SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)*7) );
				}
			}
*/

			time_t currentTime;
			time(&currentTime);
			int currentHour = localtime(&currentTime)->tm_hour;
			if (SETTING(TIME_DEPENDENT_THROTTLE) &&
				((SETTING(BANDWIDTH_LIMIT_START) < SETTING(BANDWIDTH_LIMIT_END) &&
					currentHour >= SETTING(BANDWIDTH_LIMIT_START) && currentHour < SETTING(BANDWIDTH_LIMIT_END)) ||
				(SETTING(BANDWIDTH_LIMIT_START) > SETTING(BANDWIDTH_LIMIT_END) &&
					(currentHour >= SETTING(BANDWIDTH_LIMIT_START) || currentHour < SETTING(BANDWIDTH_LIMIT_END))))
			) {
				//want to keep this out of the upload limiting code proper, where it might otherwise work more naturally
				SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME));
				SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME));
			} else {
				SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL));
				SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL));
			}
		} else {
			SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, 0);
			SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, 0);
		}		
}
#ifdef PPA_INCLUDE_CHECK_UPDATE
void MainFrame::on(HttpConnectionListener::Failed, HttpConnection* /*conn*/, string const& aLine) throw() {
#ifdef _DEBUG
  debugTrace("Update check failed: %s\n", aLine.c_str());
#endif
  StringMap params;
  params["message"] = "Update check failed: " + aLine;
  LOG(LogManager::SYSTEM, params);
}

void MainFrame::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
	versionInfo += string((const char*)buf, len);
}
#endif

void MainFrame::on(PartialList, const UserPtr& aUser, const string& text) throw() {
	PostMessage(WM_SPEAKER, BROWSE_LISTING, (LPARAM)new DirectoryBrowseInfo(aUser, text));
}

void MainFrame::on(QueueManagerListener::Finished, QueueItem* qi, int64_t speed) throw() {
	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			// This is a file listing, show it...
			DirectoryListInfo* i = new DirectoryListInfo(qi->getCurrents()[0], Text::toT(qi->getListName()), speed);

			PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
		} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
			PostMessage(WM_SPEAKER, VIEW_FILE_AND_DELETE, (LPARAM) new tstring(Text::toT(qi->getTarget())));
		}
	} else if(qi->isSet(QueueItem::FLAG_USER_LIST) && qi->isSet(QueueItem::FLAG_CHECK_FILE_LIST)) {
                DirectoryListInfo* i = new DirectoryListInfo(qi->getCurrents()[0], Text::toT(qi->getListName()).c_str(), speed);
		
		if(listQueue.stop) {
			listQueue.stop = false;
			listQueue.start();
		}
		{
			Lock l(listQueue.cs);
			listQueue.fileLists.push_back(i);
		}
		listQueue.s.signal();
	}	
}

void MainFrame::on(QueueManagerListener::OnlineVideoReady, QueueItem* qi) throw() {
	string path = qi->getTempTarget();
	if (!path.empty() && File::exists(path)) {
		WinUtil::showVideo(path);
		return;
	}
	WinUtil::showVideo(qi->getTarget());
}

LRESULT MainFrame::onActivateApp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  bHandled = FALSE;
  WinUtil::isAppActive = (wParam == TRUE);	//wParam == TRUE if window is activated, FALSE if deactivated
  if (wParam == TRUE) {
    MDIContainer::Window activeChild = MDIContainer::getActive();
    if (activeChild) {
      activeChild->SendMessage(WPM_APP_ACTIVATE);
    }
    if (bIsPM && bTrayIcon == true) {
      setIcon(m_normalicon);
      bIsPM = false;
    }
  }
  return 0;
}

LRESULT MainFrame::onShowWindow(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (wParam == TRUE) {
    // для надежности (пере-)создания банера - иногда он не создавался.
    MDIContainer::Window activeChild = MDIContainer::getActive();
    if (activeChild) {
      activeChild->SendMessage(WPM_APP_ACTIVATE);
    }
  }
  return 0;
}

LRESULT MainFrame::onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)
		ctrlTab.SwitchTo();
	if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_BACKWARD)
		ctrlTab.SwitchTo(false);
	
	return TRUE;
}

LRESULT MainFrame::onAway(WORD , WORD , HWND, BOOL& ) {
	if(Util::getAway()) { 
		setAwayButton(false);
		Util::setAway(false);
	} else {
		setAwayButton(true);
		Util::setAway(true);
	}
	ClientManager::getInstance()->infoUpdated(true);
	return 0;
}

LRESULT MainFrame::onDisableSounds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SettingsManager::getInstance()->set(SettingsManager::SOUNDS_DISABLED, !BOOLSETTING(SOUNDS_DISABLED));
	UISetCheck(IDC_ENABLE_SOUNDS, !BOOLSETTING(SOUNDS_DISABLED));
	return 0;
}

void MainFrame::on(WebServerListener::Setup) throw() {
 	WSAAsyncSelect(WebServerManager::getInstance()->getServerSocket().getSock(), m_hWnd, WEBSERVER_SOCKET_MESSAGE, FD_ACCEPT);
}

void MainFrame::on(WebServerListener::ShutdownPC, int action) throw() {
	WinUtil::shutDown(action);
}

int MainFrame::FileListQueue::run() {
	setThreadPriority(Thread::LOW);

	while(true) {
		s.wait(15000);
		if(stop || fileLists.empty()) {
			break;
		}

		DirectoryListInfo* i;
		{
			Lock l(cs);
			i = fileLists.front();
			fileLists.pop_front();
		}
		if(Util::fileExists(Text::fromT(i->file))) {
			DirectoryListing* dl = new DirectoryListing(i->user);
			try {
				dl->loadFile(Text::fromT(i->file));
				ADLSearchManager::getInstance()->matchListing(*dl);
				ClientManager::getInstance()->checkCheating(i->user, dl);
			} catch(...) {
			}
			delete dl;
		}
		delete i;
	}
	stop = true;
	return 0;
}

LRESULT MainFrame::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    if (shutdownSplash) {
      shutdownSplash->DestroyWindow();
      delete shutdownSplash;
      shutdownSplash = NULL;
    }
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    if (pLoop != NULL) {
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
    }
	LogManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);

	if(bTrayIcon) {
		updateTray(false);
	}
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onTrayLimiter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(BOOLSETTING(THROTTLE_ENABLE) == true) {
		Util::setLimiter(false);
		MainFrame::setLimiterButton(false);
	} else {
		Util::setLimiter(true);
		MainFrame::setLimiterButton(true);
	}
	ClientManager::getInstance()->infoUpdated(true);
	return 0;
}

// !SMT!-UI
void MainFrame::setIcon(HICON newIcon)
{
	NOTIFYICONDATA nid={0};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON;
	nid.hIcon = newIcon;
	::Shell_NotifyIcon(NIM_MODIFY, &nid);
}

LRESULT MainFrame::onLockToolbars(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UISetCheck(IDC_LOCK_TOOLBARS, !BOOLSETTING(LOCK_TOOLBARS));
	SettingsManager::getInstance()->set(SettingsManager::LOCK_TOOLBARS, !BOOLSETTING(LOCK_TOOLBARS));
	toggleLockToolbars();
	return 0;
}

LRESULT MainFrame::onChat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  MDIContainer::Window window = MDIContainer::getActive();
  if (window) {
    if (HubFrameFactory::activateThisChat(window)) {
      return 0;
    }
  }
#ifdef _DEBUG
  else {
    dcassert(WinUtil::tabCtrl->getCount() == 0);
  }
#endif
  HubFrameFactory::activateAnyChat();
  return 0;
}

LRESULT MainFrame::onEgoPhone(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  EgoPhoneLauncher launcher(m_hWnd);
  launcher.execute();
  return 0;
}

LRESULT MainFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (BOOLSETTING(MENU_ADVANCED)) {
    if(reinterpret_cast<HWND>(wParam) == m_hWndToolBar) {
      POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
      tbMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

      return TRUE; 
    }
  }
  bHandled = FALSE;
  return FALSE;
}

void MainFrame::toggleTopmost() const {
	DWORD dwExStyle = (DWORD)GetWindowLong(GWL_EXSTYLE);
	CRect rc;
	GetWindowRect(rc);
	HWND order = (dwExStyle & WS_EX_TOPMOST) ? HWND_NOTOPMOST : HWND_TOPMOST;
	::SetWindowPos(m_hWnd, order, rc.left, rc.top, rc.Width(), rc.Height(), SWP_SHOWWINDOW);
}

void MainFrame::toggleLockToolbars() const {
	CReBarCtrl rbc = m_hWndToolBar;
	REBARBANDINFO rbi;
	rbi.cbSize = sizeof(rbi);
	rbi.fMask  = RBBIM_STYLE;
	int nCount  = rbc.GetBandCount();
	for (int i  = 0; i < nCount; i++) {
		rbc.GetBandInfo(i, &rbi);
		rbi.fStyle ^= RBBS_NOGRIPPER | RBBS_GRIPPERALWAYS;
		rbc.SetBandInfo(i, &rbi);
	}
}

LRESULT MainFrame::onSelected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  HWND hWnd = (HWND)wParam;
  if (MDIGetActive() != hWnd) {
    MDIActivate(hWnd);
  } 
  if (::IsIconic(hWnd)) {
    ::ShowWindow(hWnd, SW_RESTORE);
  }
  return 0;
}

LRESULT MainFrame::onWindowMinimizeAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
  tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
  while (tmpWnd!=NULL) {
    ::CloseWindow(tmpWnd);
    tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
  }
  return 0;
}

LRESULT MainFrame::onWindowRestoreAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
  HWND ClientWnd = tmpWnd; //saving client window handle
  tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
  BOOL bmax;
  while (tmpWnd!=NULL) {
    ::ShowWindow(tmpWnd, SW_RESTORE);
    ::SendMessage(ClientWnd,WM_MDIGETACTIVE,NULL,(LPARAM)&bmax);
    if(bmax)break; //bmax will be true if active child 
    //window is maximized, so if bmax then break
    tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
  }
  return 0;
}	

LRESULT MainFrame::onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/) {
  if (::IsIconic(m_hWnd)) {
    if(SETTING(PROTECT_TRAY) && (!maximized) && hasPassword()) {
      const TCHAR* passwordWindowTitle = _T("Password required - ") _T(APPNAME);
      HWND otherWnd = FindWindow(NULL, passwordWindowTitle);
      if(otherWnd == NULL) {
        LineDlg dlg;
        dlg.description = TSTRING(PASSWORD_DESC);
        dlg.title = passwordWindowTitle;
        dlg.password = true;
        dlg.disable = true;
        if(dlg.DoModal(/*m_hWnd*/) == IDOK) {
          tstring tmp = dlg.line;
          TigerTree mytth(TigerTree::calcBlockSize(tmp.size(), 1));
          mytth.update(tmp.c_str(), tmp.size());
          mytth.finalize();
          if(mytth.getRoot().toBase32().c_str() == SETTING(PASSWORD)) {
            ShowWindow(SW_SHOW);
            ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
          }
        }
      } else {
        ::SetForegroundWindow(otherWnd);
      }
    } else {
      ShowWindow(SW_SHOW);
      ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
    }
  }
  return 0;
}

void MainFrame::onToolbarSearchToggle() {
  UpdateLayout(TRUE);
}

bool MainFrame::hasPassword() const {
  const string password = SETTING(PASSWORD);
  return password.empty() && password != "LWPNACQDBZRYXW3VHJVCJ64QBZNGHOHHHZWCLNQ";
}

LRESULT MainFrame::onGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  DefWindowProc();
  int minChildHeight = getMinChildHeight();
  // footer
  if (m_footer) {
    RECT r;
    m_footer.GetWindowRect(&r);
    minChildHeight += r.bottom - r.top;
  }
  // toolbar
  if (m_hWndToolBar != NULL && ((DWORD)::GetWindowLong(m_hWndToolBar, GWL_STYLE) & WS_VISIBLE)) {
    RECT rectTB = { 0 };
    ::GetWindowRect(m_hWndToolBar, &rectTB);
    minChildHeight += rectTB.bottom - rectTB.top;
  }
  // status bar
  if (m_hWndStatusBar != NULL && ((DWORD)::GetWindowLong(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE)) {
    RECT rectSB = { 0 };
    ::GetWindowRect(m_hWndStatusBar, &rectSB);
    minChildHeight += rectSB.bottom - rectSB.top;
  }
  if (m_peersToolbar) {
    minChildHeight += m_peersToolbar.getHeight();
  }
  if (m_searchBar) {
	  minChildHeight += m_searchBar.getHeight();
  }
  if (ctrlTab.IsWindow()) {
    minChildHeight += ctrlTab.getHeight();
  }
  CRect clientR;
  GetClientRect(clientR);
  CRect windowR;
  GetWindowRect(windowR);
  minChildHeight += windowR.Height() - clientR.Height();
  minChildHeight += m_cxySplitBar + m_cxyBarEdge;
  if (minChildHeight > LPMINMAXINFO(lParam)->ptMinTrackSize.y) {
    LPMINMAXINFO(lParam)->ptMinTrackSize.y = minChildHeight;
  }
  const int minWidth = 900;
  if (minWidth > LPMINMAXINFO(lParam)->ptMinTrackSize.x) {
	  LPMINMAXINFO(lParam)->ptMinTrackSize.x = minWidth;
  }
  return 0;
}

/**
 * @file
 * $Id: MainFrm.cpp,v 1.72.2.8 2008/12/10 20:43:17 alexey Exp $
 */
