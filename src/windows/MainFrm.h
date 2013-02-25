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

#if !defined(MAIN_FRM_H)
#define MAIN_FRM_H

#ifdef PPA_INCLUDE_CHECK_UPDATE
#include "../client/HttpConnection.h"
#include "../peers/HttpUpdateDownloader.h"
#endif
#include "../client/FavoriteManager.h"
#include "../client/QueueManagerListener.h"
#include "../client/LogManager.h"
#include "../client/ShareManager.h"
#include "../client/WebServerManager.h"
#include "../client/AdlSearch.h"
#include "../client/HistoryManager.h"
#include "PopupManager.h"

#include "SingleInstance.h"
#include "TransferView.h"
#ifdef PPA_INCLUDE_UPNP
#include "UPnP.h"
#endif
#include "LineDlg.h"
#include "../peers/PeersVersion.h"
#include "MainFrmApi.h"
#include "../peers/ToolbarImages.h"
#include "../peers/PeersToolbar.h"
#include "../peers/PeersSearchBar.h"
#include "../peers/HubMessageControl.h"
#include "../peers/MainFooter.h"

class MainFrame : 
  public CMDIFrameWindowImpl<MainFrame>, 
  public CUpdateUI<MainFrame>,
  public CMessageFilter, 
  public CIdleHandler, 
  public CSplitterImpl<MainFrame, false>, 
  public Thread,
  private TimerManagerListener, 
#ifdef PPA_INCLUDE_CHECK_UPDATE
  private HttpConnectionListener, 
  private HttpUpdateDownloaderListener,
#endif
  private QueueManagerListener,
  private LogManagerListener,
  private WebServerListener,
  private PeersToolbarListener
{
	DWORD m_OLDPriorityClass;
#ifdef PPA_INCLUDE_CHECK_UPDATE
	enum {  
	  TIMER_CHECK_UPDATE = 1
	};
#endif

	enum {
	  PINGER_TIMER = 2 // BAD HACK
	};
public:
	MainFrame();
	virtual ~MainFrame();
	DECLARE_FRAME_WND_CLASS(_T(APPNAME), IDR_MAINFRAME)

	CMDICommandBarCtrl m_CmdBar;

        enum {
          DOWNLOAD_LISTING,
          BROWSE_LISTING,
          STATS,
          AUTO_CONNECT,
          PARSE_COMMAND_LINE,
		  ACTIVATE_ADVICE_WINDOW,
          VIEW_FILE_AND_DELETE, 
          SET_STATUSTEXT,
          STATUS_MESSAGE,
          SHOW_POPUP,
          SET_PM_TRAY_ICON,
          UPDATE_RESTART_CONFIRM
        };

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if((pMsg->message >= WM_MOUSEFIRST) && (pMsg->message <= WM_MOUSELAST))
			ctrlLastLines.RelayEvent(pMsg);

		if (!IsWindow())
			return FALSE;

		if(CMDIFrameWindowImpl<MainFrame>::PreTranslateMessage(pMsg))
			return TRUE;
		
		HWND hWnd = MDIGetActive();
		if(hWnd != NULL)
			return (BOOL)::SendMessage(hWnd, WM_FORWARDMSG, 0, (LPARAM)pMsg);

		return FALSE;
	}
	
	BOOL OnIdle()
	{
		UIUpdateToolBar();
                const HWND activeChild = MDIGetActive();
                if (activeChild) {
                  ::SendMessage(activeChild, WPM_IDLE, 0, 0);
                }
		return FALSE;
	}
	typedef CSplitterImpl<MainFrame, false> splitterBase;
	BEGIN_MSG_MAP(MainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)		
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(FTM_SELECTED, onSelected)
		MESSAGE_HANDLER(FTM_ROWS_CHANGED, onRowsChanged)
		MESSAGE_HANDLER(WPM_TRAY, onTrayIcon)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_ENDSESSION, onEndSession)
		MESSAGE_HANDLER(trayMessage, onTray)
		MESSAGE_HANDLER(WM_COPYDATA, onCopyData)
		MESSAGE_HANDLER(WMU_WHERE_ARE_YOU, onWhereAreYou)
		MESSAGE_HANDLER(WM_ACTIVATEAPP, onActivateApp)
		MESSAGE_HANDLER(WM_APPCOMMAND, onAppCommand)
		MESSAGE_HANDLER(IDC_REBUILD_TOOLBAR, OnCreateToolbar)
		MESSAGE_HANDLER(WEBSERVER_SOCKET_MESSAGE, onWebServerSocket)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SHOWWINDOW, onShowWindow)
		MESSAGE_HANDLER(WM_GETMINMAXINFO, onGetMinMaxInfo)
#ifdef PPA_INCLUDE_CHECK_UPDATE
		MESSAGE_HANDLER(WM_TIMER, onTimer)
#endif
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_SETTINGS, OnFileSettings)
		COMMAND_ID_HANDLER(IDC_FILELIST_ADD_FILE, onFileListAddFile)
		COMMAND_ID_HANDLER(IDC_MATCH_ALL, onMatchAll)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_VIEW_TRANSFER_VIEW, OnViewTransferView)
		COMMAND_ID_HANDLER(ID_GET_TTH, onGetTTH)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_WINDOW_CASCADE, OnWindowCascade)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_HORZ, OnWindowTile)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_VERT, OnWindowTileVert)
		COMMAND_ID_HANDLER(ID_WINDOW_ARRANGE, OnWindowArrangeIcons)
		COMMAND_ID_HANDLER(IDC_RECENTS, onOpenWindows)
		COMMAND_ID_HANDLER(ID_FILE_SEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FAVORITES, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FAVUSERS, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_NOTEPAD, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_ADVICE_WINDOW, onOpenWindows);
		COMMAND_ID_HANDLER(IDC_QUEUE, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_SEARCH_SPY, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FILE_ADL_SEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_NET_STATS, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FINISHED, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FINISHED_UL, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_UPLOAD_QUEUE, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_CDMDEBUG_WINDOW, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_AWAY, onAway)
		COMMAND_ID_HANDLER(IDC_LIMITER, onLimiter)
		COMMAND_ID_HANDLER(IDC_THROTTLE_ENABLE, onLimiter)
		COMMAND_ID_HANDLER(IDC_HELP_HOMEPAGE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DONATE, onLink)
		//[-]PPA COMMAND_ID_HANDLER(IDC_HELP_GEOIPFILE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DISCUSS, onLink)
		COMMAND_ID_HANDLER(IDC_GUIDES, onLink)
		COMMAND_ID_HANDLER(IDC_SITES_TAN, onLink)
		COMMAND_ID_HANDLER(IDC_SITES_FLYLINK_TRAC, onLink)
		COMMAND_ID_HANDLER(IDC_SITES_NXP, onLink)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE_LIST, onOpenFileList)
		COMMAND_ID_HANDLER(IDC_OPEN_MY_LIST, onOpenFileList)
		COMMAND_ID_HANDLER(IDC_TRAY_SHOW, onAppShow)
		COMMAND_ID_HANDLER(ID_WINDOW_MINIMIZE_ALL, onWindowMinimizeAll)
		COMMAND_ID_HANDLER(ID_WINDOW_RESTORE_ALL, onWindowRestoreAll)
		COMMAND_ID_HANDLER(IDC_SHUTDOWN, onShutDown)
		COMMAND_ID_HANDLER(IDC_DISABLE_SOUNDS, onDisableSounds)
		COMMAND_ID_HANDLER(IDC_ENABLE_SOUNDS, onDisableSounds)
		COMMAND_ID_HANDLER(IDC_CLOSE_DISCONNECTED, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_PM, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_OFFLINE_PM, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_DIR_LIST, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_SEARCH_FRAME, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_RECONNECT_DISCONNECTED, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_HUBS, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_HUBS_BELOW, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_HUBS_NO_USR, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_OPEN_DOWNLOADS, onOpenDownloads)
		COMMAND_ID_HANDLER(IDC_REFRESH_FILE_LIST, onRefreshFileList)
		COMMAND_ID_HANDLER(ID_FILE_QUICK_CONNECT, onQuickConnect)
		COMMAND_ID_HANDLER(IDC_HASH_PROGRESS, onHashProgress)
		COMMAND_ID_HANDLER(IDC_TRAY_LIMITER, onTrayLimiter)
		COMMAND_ID_HANDLER(ID_TOGGLE_TOOLBAR, OnViewWinampBar)
		COMMAND_ID_HANDLER(IDC_TOPMOST, OnViewTopmost)
		COMMAND_ID_HANDLER(IDC_LOCK_TOOLBARS, onLockToolbars)
		COMMAND_HANDLER(IDC_PEERS_TOOLBAR_CHAT, BN_CLICKED, onChat);
		COMMAND_HANDLER(IDC_PEERS_TOOLBAR_EGOPHONE, BN_CLICKED, onEgoPhone);
		COMMAND_RANGE_HANDLER(IDC_WINAMP_BACK, IDC_WINAMP_VOL_HALF, onWinampButton)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		CHAIN_MDI_CHILD_COMMANDS()
		CHAIN_MSG_MAP(CUpdateUI<MainFrame>)
		CHAIN_MSG_MAP(CMDIFrameWindowImpl<MainFrame>)
		CHAIN_MSG_MAP(splitterBase);
	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(MainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_TRANSFER_VIEW, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TOGGLE_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_TRAY_LIMITER, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_TOPMOST, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_LOCK_TOOLBARS, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_SHUTDOWN, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_ENABLE_SOUNDS, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_AWAY, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(IDC_THROTTLE_ENABLE, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()


	LRESULT onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onHashProgress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFileListAddFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCloseWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);	
	LRESULT onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onActivateApp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onWebServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onAway(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLimiter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDisableSounds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenWindows(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTrayLimiter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewWinampBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWinampButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnViewTopmost(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onLockToolbars(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEgoPhone(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);

	static DWORD WINAPI stopper(void* p);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void parseCommandLine(const tstring& cmdLine);

#ifdef PPA_INCLUDE_UPNP
	void startUPnP();
	void stopUPnP();
#endif
	LRESULT onWhereAreYou(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return WMU_WHERE_ARE_YOU;
	}

	LRESULT onTray(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
		bTrayIcon = false;
		updateTray(true); 
		return 0;
	}

	LRESULT onRowsChanged(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		UpdateLayout();
		Invalidate();
		return 0;
	}

	LRESULT onSelected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	
	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 0;
	}
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		closeEnabled = true;
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	LRESULT onOpenDownloads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		WinUtil::openFile(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
		return 0;
	}

	LRESULT OnWindowCascade(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDICascade();
		return 0;
	}

	LRESULT OnWindowTile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile();
		return 0;
	}

	LRESULT OnWindowTileVert(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile(MDITILE_VERTICAL);
		return 0;
	}

	LRESULT OnWindowArrangeIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDIIconArrange();
		return 0;
	}

	LRESULT onWindowMinimizeAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWindowRestoreAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onShutDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		setShutDown(!getShutDown());
		UISetCheck(IDC_SHUTDOWN, getShutDown());
		return S_OK;
	}
	LRESULT OnCreateToolbar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		createToolbar();
		return S_OK;
	}
	static MainFrame* getMainFrame() { return anyMF; }
	bool getAppMinimized() const { return bAppMinimized; }
	CToolBarCtrl& getToolBar() { return ctrlToolbar; }

	static void setShutDown(bool b) {
		if (b)
			iCurrentShutdownTime = TimerManager::getTick() / 1000;
		bShutdown = b;
	}
	static bool getShutDown() { return bShutdown; }

	static void setAwayButton(bool check) {
		anyMF->ctrlToolbar.CheckButton(IDC_AWAY, check);
                anyMF->UISetCheck(IDC_AWAY, check);
	}

	static void setLimiterButton(bool check) {
		anyMF->ctrlToolbar.CheckButton(IDC_LIMITER, check);
		anyMF->UISetCheck(IDC_TRAY_LIMITER, check);
		anyMF->UISetCheck(IDC_THROTTLE_ENABLE, check);
	}

	ToolbarImages m_toolbarImages;
	CImageListContainer winampImages, winampImagesHot;
	virtual int run();
	bool SetSplitterPos(int xyPos = -1, bool bUpdate = true);


private:
	CIcon m_normalicon;
	CIcon m_pmicon;

	bool getPassword(); // !SMT!-f

	class DirectoryListInfo {
	public:
		DirectoryListInfo(const UserPtr& aUser, const tstring& aFile, int64_t aSpeed) : user(aUser), file(aFile), speed(aSpeed) { }
		UserPtr user;
		tstring file;
		int64_t speed;
	};
	class DirectoryBrowseInfo {
	public:
		DirectoryBrowseInfo(const UserPtr& ptr, string aText) : user(ptr), text(aText) { }
		UserPtr user;
		string text;
	};
	class FileListQueue: public Thread {
	public:
		bool stop;
		Semaphore s;
		CriticalSection cs;
		list<DirectoryListInfo*> fileLists;

		FileListQueue() : stop(true) {}
		~FileListQueue() throw() {
			shutdown();
		}

		int run();
		void shutdown() {
			stop = true;
			s.signal();
		}
	};
	
	friend class TransferView;
	friend class MainFooter;
	TransferView transferView;
	bool splitterResizeActive;
	static MainFrame* anyMF;

	enum { MAX_CLIENT_LINES = 10 };
	TStringList lastLinesList;
	tstring lastLines;
	CToolTipCtrl ctrlLastLines;
	CStatusBarCtrl ctrlStatus;
	FlatTabCtrl ctrlTab;
	PeersToolbar m_peersToolbar;
	PeersSearchBar m_searchBar;
	HubMessageControl m_hubMessages;
	MainFooter m_footer;
#ifdef PPA_INCLUDE_CHECK_UPDATE
	HttpConnection* updatesChecker;
	HttpConnection *pinger;
	string versionInfo;
	auto_ptr<HttpUpdateDownloader> downloader;
#endif
	CImageListContainer images;
	CToolBarCtrl ctrlToolbar;
	CToolBarCtrl ctrlWinampToolbar;

	bool tbarcreated;
	bool awaybyminimize;
	bool bTrayIcon;
	bool bAppMinimized;
	bool bIsPM;
	
	static bool bShutdown;
	static uint64_t iCurrentShutdownTime;
	HICON hShutdownIcon;
	static bool isShutdownStatus;

	CMenu trayMenu;
	CMenu tbMenu;

	UINT trayMessage;
	/** Was the window maximized when minimizing it? */
	bool maximized;
	uint64_t lastMove;
	uint32_t lastUpdate;
	int64_t lastUp;
	int64_t lastDown;
	tstring lastTTHdir;
	/* выход для выполнения обновления - не нужно ничего спрашивать */
	bool exitToUpdate;
	bool closeEnabled;
	bool closing;
	int lastUpload;
	int statusSizes[10];
	HANDLE stopperThread;
	FileListQueue listQueue;
	bool missedAutoConnect;
	HWND createToolbar();
	HWND createWinampToolbar();
	HWND hWndCmdBar;
	void updateTray(bool add = true);
	void toggleTopmost() const;
	void toggleLockToolbars() const;
	void updateQuickSearches(const tstring& search = Util::emptyStringT);
	void saveWindowPlacement();

	LRESULT onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/);
	int getMinChildHeight() const;

	void autoConnect(const FavoriteHubEntry::List& fl);
	void startSocket();

	void setIcon(HICON newIcon); // !SMT!-UI
	MainFrame(const MainFrame&):m_searchBar(NULL) { dcassert(0); }

	/* проверяет наличие установленного пароля */
	bool hasPassword() const;

	// LogManagerListener
	void on(LogManagerListener::Message, const string& m) throw() { PostMessage(WM_SPEAKER, STATUS_MESSAGE, (LPARAM)new tstring(Text::toT(m))); }

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second type, uint32_t aTick) throw();
	
#ifdef PPA_INCLUDE_CHECK_UPDATE
	// HttpConnectionListener
	void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/) throw();
	void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw();	
	void on(HttpConnectionListener::Failed, HttpConnection* /*conn*/, string const& /*aLine*/) throw();
	void onDownloadComplete(HttpUpdateDownloader* downloader) throw();
	LRESULT onTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	bool isMainUpdate(SimpleXML& xml);
#endif
	// WebServerListener
 	void on(WebServerListener::Setup);
	void on(WebServerListener::ShutdownPC, int);

	// QueueManagerListener
	void on(QueueManagerListener::Finished, QueueItem* qi, int64_t speed) throw();
	void on(PartialList, const UserPtr&, const string& text) throw();
	void on(OnlineVideoReady, QueueItem* qi) throw();

	// PeersToolbarListener
	virtual void onToolbarSearchToggle();

	void detectNetworkSetup();

#ifdef PPA_INCLUDE_UPNP
	// UPnP connectors
	UPnP* UPnP_TCPConnection;
	UPnP* UPnP_UDPConnection;
#endif
};

#endif // !defined(MAIN_FRM_H)

/**
 * @file
 * $Id: MainFrm.h,v 1.35.2.3 2008/12/04 20:20:38 alexey Exp $
 */
