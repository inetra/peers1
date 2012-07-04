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

#if !defined(FLAT_TAB_CTRL_H)
#define FLAT_TAB_CTRL_H

enum {
	FT_FIRST = WM_APP + 700,
	/** This will be sent when the user presses a tab. WPARAM = HWND */
	FTM_SELECTED,
	/** The number of rows changed */
	FTM_ROWS_CHANGED,
	/** Set currently active tab to the HWND pointed by WPARAM */
	FTM_SETACTIVE,
	/** Display context menu and return TRUE, or return FALSE for the default one */
	FTM_CONTEXTMENU,
	/** Close window with postmessage... */
	WM_REALLY_CLOSE
};

class MDIContainer {
public:
  typedef CFrameWindowImplBase<CMDIWindow, CMDIChildWinTraits>* Window;
  static void onCreate(HWND hwnd, Window child);
  static void onDestroy(HWND hwnd);
  static Window get(HWND hwnd);
  static Window getActive();
  static vector<Window> list();
};

class ATL_NO_VTABLE WindowListener {
public:
  virtual void windowClosed(MDIContainer::Window window) = 0;
};

enum WindowSortOrders {
	SORT_ORDER_PEERS,
	SORT_ORDER_ADVICE,
	SORT_ORDER_SUBSCRIPTIONS,
	SORT_ORDER_PEERS_CHAT,
	SORT_ORDER_HUB_OTHER,
	SORT_ORDER_OTHER
};

class WindowSortOrder {
public:
  virtual WindowSortOrders getSortOrder() = 0;
};

class FlatTabCtrl : public CWindowImpl<FlatTabCtrl, CWindow, CControlWinTraits> {
private:
  int topMargin;
  int leftMargin;
  int rightMargin;
  int bottomMargin; /* включая линию */
  int horzGap;
  int vertGap;
  int activeTabDelta;
  int getMaxRightPos() const;
  bool hasDirtyTabs() const;
  bool m_dirtyFlashOn;
public:
  FlatTabCtrl();
  virtual ~FlatTabCtrl();

  void addTab(MDIContainer::Window window, bool largeIcon, int icon = 0, int stateIcon = 0);
  void removeTab(HWND aWnd);
  void startSwitch();
  void endSwitch();
  HWND getNext();
  HWND getPrev();
  void setActive(HWND aWnd);
  void setTop(HWND aWnd);
  void setDirty(HWND aWnd);
  void setIconState(HWND aWnd);
  void unsetIconState(HWND aWnd);
  // !SMT!-UI
  void setCustomIcon(HWND aWnd, HICON custom);
#if 0
  void setColor(HWND aWnd, COLORREF color);
#endif
  void updateText(HWND aWnd, LPCTSTR text);

  int getTabHeight() const { return tabHeight; }
  /* возвращает высоту, которую бы хотел занимать этот контрол */
  int getHeight() const;
  int getRows() const { return rows; }
  void calcRows(bool inval = true);
  void SwitchTo(bool next = true);

  /* возвращает количество табов */
  int getCount() const { return tabs.size(); }

  BEGIN_MSG_MAP(thisClass)
    MESSAGE_HANDLER(WM_SIZE, onSize)
    MESSAGE_HANDLER(WM_CREATE, onCreate)
    MESSAGE_HANDLER(WM_PAINT, onPaint)
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, onLButtonUp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
    MESSAGE_HANDLER(WM_MOUSEMOVE, onMouseMove)		
    MESSAGE_HANDLER(WM_MBUTTONUP, onCloseTab)
    MESSAGE_HANDLER(WM_TIMER, onTimer)
    COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
    COMMAND_ID_HANDLER(IDC_CHEVRON, onChevron)
    COMMAND_RANGE_HANDLER(IDC_SELECT_WINDOW, IDC_SELECT_WINDOW+tabs.size(), onSelectWindow)
  END_MSG_MAP()

  LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onChevron(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onSelectWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onCloseTab(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
private:
  class TabInfo {
  public:

    typedef vector<TabInfo*> List;
    typedef List::const_iterator ListIter;

    enum { MAX_LENGTH = 20 };

    TabInfo(MDIContainer::Window window, bool largeIcon, int icon, int stateIcon);
    ~TabInfo();

    /* идентификатор окна, к которому относится этот таб */
	HWND hWnd;
    MDIContainer::Window m_window;
    /* название таба */
    TCHAR name[MAX_LENGTH];
    /* длина названия таба */
    size_t len;
    
    /* ширина таба, включая линию */
    int width;
    /* координата X */
    int xpos;
    /* номер строки */
    int row;
    
    /* признак наличия информации, которую еще не видел пользователь */
    bool dirty;
    
    /* рисовать большой значок */
    bool m_largeIcon;
    /* индекс основного значка */
    int m_iconIndex;
    /* индекс альтернативного значка */
    int m_altIconIndex;
    /* значок, заданный внешним кодом */
    HICON hCustomIcon; // !SMT!-UI custom icon should be set / freed outside this class
    /* использовать альтернативный значок или основной */
    bool bState;

    bool update();

    bool updateText(const LPCTSTR text);

    int getWidth() const {
      return width;
    }
  };

  void moveTabs(const TabInfo* aTab, bool after);
  /* возвращает HICON соответствующий указанному индексу */
  HICON getIconByIndex(int index);
  /* загружает значок с указанным идентификатором, возвращает индекс по которому его можно будет отображать или 0 */
  int loadIcon(int resourceId);

  typedef map<int,HICON> IconMap;
  IconMap m_icons;

  /* окно для которого сейчас отображается popup-меню с командой Close */
  HWND closing;
  /* кнопка для вызова popup-меню списка табов, которые не входят в настроенное количество строк */
  CButton chevron;
  /* popup-меню списка табов, которое отображается при помощи кнопки chevron */
  CMenu mnu;
  /* вероятно для отображения хинта с полным названием таба при наведении мышки */
  CToolTipCtrl tab_tip;

  /* текущее количество строк табов */
  int rows;
  /* высота таба */
  int tabHeight;
  /* ширина символа */
  int charWidth;

  /* активный таб */
  TabInfo* active;
  /* таб у которого сейчас над кнопкой close находится мышь */
  TabInfo* closeHoverTab;
  /* таб, который в настоящее время перемещается */
  TabInfo* moving;
  /* список табов */
  TabInfo::List tabs;

  CBitmap bmpCloseActive;
  CBitmap bmpCloseInactive;
  CBitmap bmpCloseHover;
  CIcon icoTabDirty;

  typedef list<HWND> WindowList;
  typedef WindowList::const_iterator WindowIter;

  /* список табов/окон в порядке отображения. непонятно зачем - можно же менять порядок в основном списке */
  WindowList viewOrder;
  /* что-то непонятное... */
  WindowIter nextTab;

  /* устанавливается в методе startSwitch(), 
     сбрасывается в методе endSwitch(), 
     которые как-то связаны с нажатием кнопки Ctrl */
  bool inTab;

  /* находит tab по идентификатору окна. возвращает найденный tab или NULL */
  TabInfo* getTabInfo(HWND aWnd) const;

  /* находит tab по координатам. возвращает найденный tab или NULL */
  TabInfo* findTab(int x, int y) const;

  /* определяет координаты таба в контроле */
  void getTabRect(const TabInfo* tab, LPRECT rect) const;

  static inline bool compareTabInfoByPosX(TabInfo *tab1, TabInfo *tab2) {
    return tab1->xpos > tab2->xpos ? true : false;
  }

  WindowSortOrders getSortOrderOf(MDIContainer::Window window);

  /**
  * Draws a tab
  */
  void drawTab(CDCHandle dc, const TabInfo* tab, int ypos, bool aActive = false);

  void addTabTool(const TabInfo* tab);

  void deleteTabTool(const TabInfo* tab);

  void updateTabToolText(const TabInfo* tab, LPCTSTR tabText = NULL);

  void updateTabToolRect(const TabInfo* tab);

  bool isCloseHover(const TabInfo* tab, int x, int y) const;
};

template <class T, int I = 0, int I_state = 0>
class ATL_NO_VTABLE MDITabChildWindowImpl : public CMDIChildWindowImpl<T, CMDIWindow, CMDIChildWinTraits> {
private:
  typedef list<WindowListener*> ListenerList;
  ListenerList listeners;
public:

	MDITabChildWindowImpl() : created(false) { }
	FlatTabCtrl* getTab() { return WinUtil::tabCtrl; }

	virtual void OnFinalMessage(HWND /*hWnd*/) { delete this; }
	
 	typedef MDITabChildWindowImpl<T, I, I_state> thisClass;
	typedef CMDIChildWindowImpl<T, CMDIWindow, CMDIChildWinTraits> baseClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SYSCOMMAND, onSysCommand)
		MESSAGE_HANDLER(WM_FORWARDMSG, onForwardMsg)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_MDIACTIVATE, onMDIActivate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SETTEXT, onSetText)
		MESSAGE_HANDLER(WM_GETMINMAXINFO, onGetMinMaxInfo)
		MESSAGE_HANDLER(WM_REALLY_CLOSE, onReallyClose)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		MESSAGE_HANDLER(WPM_APP_ACTIVATE, onAppActivate)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
	
	HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
	DWORD dwStyle = 0, DWORD dwExStyle = 0,
	UINT nMenuID = 0, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		if(nMenuID != 0)
#if (_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(ATL::_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#else //!(_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#endif //!(_ATL_VER >= 0x0700)

		if(m_hMenu == NULL)
			m_hMenu = WinUtil::mainMenu;

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		dwExStyle |= WS_EX_MDICHILD;	// force this one
		m_pfnSuperWindowProc = ::DefMDIChildProc;
		m_hWndMDIClient = hWndParent;
		ATLASSERT(::IsWindow(m_hWndMDIClient));

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &CWindow::rcDefault;

		// If the currently active MDI child is maximized, we want to create this one maximized too
		ATL::CWindow wndParent = hWndParent;
		BOOL bMaximized = FALSE;

		if(MDIGetActive(&bMaximized) == NULL)
			bMaximized = BOOLSETTING(MDI_MAXIMIZED);

		if(bMaximized)
			wndParent.SetRedraw(FALSE);

		HWND hWnd = CFrameWindowImplBase<CMDIWindow,CMDIChildWinTraits>::Create(hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, (UINT)0U, atom, lpCreateParam);

		if(bMaximized)
		{
			// Maximize and redraw everything
			if(hWnd != NULL)
				MDIMaximize(hWnd);
			wndParent.SetRedraw(TRUE);
			wndParent.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
			::SetFocus(GetMDIFrame());   // focus will be set back to this window
		}
		else if(hWnd != NULL && WinUtil::isAppActive && ::IsWindowVisible(m_hWnd) && !::IsChild(hWnd, ::GetFocus()))
		{
			::SetFocus(hWnd);
		}

		return hWnd;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif		
	}

	// All MDI windows must have this in wtl it seems to handle ctrl-tab and so on...
	LRESULT onForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		return baseClass::PreTranslateMessage((LPMSG)lParam);
	}

	LRESULT onSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == SC_NEXTWINDOW) {
			HWND next = getTab()->getNext();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		} else if(wParam == SC_PREVWINDOW) {
			HWND next = getTab()->getPrev();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onCreate(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
          if (!created) {
            MDIContainer::onCreate(m_hWnd, this);
            bHandled = FALSE;
            dcassert(getTab());
            getTab()->addTab(this, isLargeIcon(), I, I_state);
            created = true;
          }
          return 0;
	}

        virtual bool isLargeIcon() const { return false; }

        virtual void onActivate() { }
        virtual void onActivate(bool /*childActivate*/) { onActivate(); }
		virtual void onDeactivate() { }

        LRESULT onMDIActivate(UINT /*uMsg*/, WPARAM /*wParam */, LPARAM lParam, BOOL& bHandled) {
          dcassert(getTab());
          if ((m_hWnd == (HWND)lParam)) {
            getTab()->setActive(m_hWnd);
            onActivate(true);
          }
		  else {
			  onDeactivate();
		  }
          bHandled = FALSE;
          return 1; 
        }

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
            MDIContainer::onDestroy(m_hWnd);
            for (ListenerList::iterator i = listeners.begin(); i != listeners.end(); ++i) {
              (*i)->windowClosed(this);
            }
		bHandled = FALSE;
		dcassert(getTab());
		getTab()->removeTab(m_hWnd);
		if(m_hMenu == WinUtil::mainMenu)
			m_hMenu = NULL;

		BOOL bMaximized = FALSE;
		if(::SendMessage(m_hWndMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized) != NULL)
			SettingsManager::getInstance()->set(SettingsManager::MDI_MAXIMIZED, (bMaximized>0));

		return 0;
	}

	LRESULT onReallyClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		MDIDestroy(m_hWnd);
		return 0;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled */) {
		PostMessage(WM_REALLY_CLOSE);
		return 0;
	}

	LRESULT onSetText(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		if(created) {
			getTab()->updateText(m_hWnd, (LPCTSTR)lParam);
		}
		return 0;
	}

	LRESULT onGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		DefWindowProc();
		LPMINMAXINFO lpMMI = (LPMINMAXINFO) lParam;
		if (Util::isVista()) {
			lpMMI->ptMaxTrackSize.x = max(lpMMI->ptMaxTrackSize.x, lpMMI->ptMaxSize.x); 
			lpMMI->ptMaxTrackSize.y = max(lpMMI->ptMaxTrackSize.y, lpMMI->ptMaxSize.y); 
		}
		return 0;
	}

	LRESULT onKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			getTab()->startSwitch();
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == VK_CONTROL) {
			getTab()->endSwitch();
		}
		bHandled = FALSE;
		return 0;
		
	}

	LRESULT onAppActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		onActivate(false);
		return 0;
	}

	void setDirty() {
		dcassert(getTab());
		getTab()->setDirty(m_hWnd);
	}
	void setIconState() {
		dcassert(getTab());
		getTab()->setIconState(m_hWnd);
	}
	void unsetIconState() {
		dcassert(getTab());
		getTab()->unsetIconState(m_hWnd);
	}
        // !SMT!-UI
        void setCustomIcon(HICON custom) {
                dcassert(getTab());
                getTab()->setCustomIcon(m_hWnd, custom);
        }

        void addWindowListener(WindowListener* listener) {
          listeners.push_back(listener);
        }

        void removeWindowListener(WindowListener* listener) {
          listeners.remove(listener);
        }

private:
	bool created;
};

#endif // !defined(FLAT_TAB_CTRL_H)

/**
 * @file
 * $Id: flattabctrl.h,v 1.23.2.1 2008/11/04 15:56:25 alexey Exp $
 */
