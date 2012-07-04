#include "stdafx.h"
#include "../peers/ControlAdjuster.h"
#ifdef max
#error max should not be defined
#endif

#define CLOSE_BUTTON_WIDTH        14
#define CLOSE_BUTTON_HEIGHT       14
#define CLOSE_BUTTON_TOP_MARGIN    2
#define CLOSE_BUTTON_RIGHT_MARGIN  2

#define ICON_WIDTH         16
#define ICON_HEIGHT        16
#define ICON_LEFT_MARGIN    2
#define ICON_TOP_MARGIN     2

#define LARGE_ICON_WIDTH   32
#define LARGE_ICON_HEIGHT  32

/* ширина таба в символах */
#define STANDARD_TAB_WIDTH  30
#define MINIMAL_TAB_WIDTH   15

// идентификатор таймера по которую будут мигать признаки наличия изменений
#define DIRTY_FLASH_TIMER_ID 1
#define DIRTY_FLASH_TIMER_INTERVAL 667 

FlatTabCtrl::FlatTabCtrl(): 
topMargin(1),
leftMargin(2),
rightMargin(2),
bottomMargin(4),
horzGap(2),
vertGap(1),
activeTabDelta(3),
closing(NULL), 
rows(1), 
tabHeight(0), 
active(NULL), 
moving(NULL), 
inTab(false) 
{ 
  bmpCloseActive.LoadBitmap(IDB_CLOSE_ACTIVE);
  bmpCloseInactive.LoadBitmap(IDB_CLOSE_INACTIVE);
  bmpCloseHover.LoadBitmap(IDB_CLOSE_HOVER);
  icoTabDirty.LoadIcon(IDI_TAB_DIRTY);
}

FlatTabCtrl::~FlatTabCtrl() {
  for (IconMap::iterator i = m_icons.begin(); i != m_icons.end(); ++i) {
    DestroyIcon(i->second);
  }
}

int FlatTabCtrl::loadIcon(int resourceId) {
  if (resourceId > 0) {
    IconMap::const_iterator i = m_icons.find(resourceId);
    if (i == m_icons.end()) {
      const HICON hIcon = (HICON) LoadImage((HINSTANCE)::GetWindowLong(m_hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
      m_icons.insert(make_pair(resourceId, hIcon));
      return hIcon ? resourceId : 0;
    }
    else {
      return i->second ? resourceId : 0;
    }
  }
  return 0;
}

HICON FlatTabCtrl::getIconByIndex(int index) {
  if (index > 0) {
    IconMap::const_iterator i = m_icons.find(index);
    if (i != m_icons.end()) {
      return i->second;
    }
  }
  return NULL;
}

/* возвращает высоту, которую бы хотел занимать этот контрол */
int FlatTabCtrl::getHeight() const { 
  return getRows() * getTabHeight() + topMargin + bottomMargin + activeTabDelta + vertGap * (getRows() - 1);
}  

WindowSortOrders FlatTabCtrl::getSortOrderOf(MDIContainer::Window window) {
	WindowSortOrder* sortOrder = dynamic_cast<WindowSortOrder*>(window);
	if (sortOrder != NULL) {
		return sortOrder->getSortOrder();
	}
	else {
		return SORT_ORDER_OTHER;
	}
}

void FlatTabCtrl::addTab(MDIContainer::Window window, bool largeIcon, int icon, int stateIcon) {
	HWND hWnd = window->m_hWnd;
  dcassert(getTabInfo(hWnd) == NULL);
  TabInfo* tab = new TabInfo(window, largeIcon, loadIcon(icon), loadIcon(stateIcon));
  addTabTool(tab);
  WindowSortOrders newSortOrder = getSortOrderOf(window);
  TabInfo::List::iterator insertionPoint = NULL;
  for (TabInfo::List::iterator i = tabs.begin(); i != tabs.end(); ++i) {
	  WindowSortOrders order = getSortOrderOf((*i)->m_window);
	  if (newSortOrder >= order) {
		  insertionPoint = i + 1;
	  }
	  else if (newSortOrder < order) {
		  insertionPoint = i;
		  break;
	  }
  }
  if (insertionPoint == NULL) {
    tabs.push_back(tab);
    viewOrder.push_back(hWnd);
    nextTab = --viewOrder.end();
  }
  else {
    tabs.insert(insertionPoint, tab);
    viewOrder.insert(viewOrder.begin(), hWnd);
    nextTab = ++viewOrder.begin();
  }
  active = tab;
  calcRows(false);
  Invalidate();
}

void FlatTabCtrl::removeTab(HWND aWnd) {
  TabInfo::List::iterator i;
  for (i = tabs.begin(); i != tabs.end(); ++i) {
    if ((*i)->hWnd == aWnd) {
      break;
    }
  }
  dcassert(i != tabs.end());
  TabInfo* ti = *i;
  if (active == ti) {
    active = NULL;
  }
  if (moving == ti) {
    moving = NULL;
  }
  if (closeHoverTab == ti) {
    closeHoverTab = NULL;
  }
  deleteTabTool(ti);
  delete ti;
  tabs.erase(i);
  viewOrder.remove(aWnd);
  nextTab = viewOrder.end();
  if(!viewOrder.empty())
    --nextTab;

  calcRows(false);
  Invalidate();
}

void FlatTabCtrl::startSwitch() {
  if(!viewOrder.empty())
    nextTab = --viewOrder.end();
  inTab = true;
}

void FlatTabCtrl::endSwitch() {
  inTab = false;
  if(active) 
    setTop(active->hWnd);
}

HWND FlatTabCtrl::getNext() {
  if(viewOrder.empty())
    return NULL;
  if(nextTab == viewOrder.begin()) {
    nextTab = --viewOrder.end();
  } else {
    --nextTab;
  }
  return *nextTab;
}

HWND FlatTabCtrl::getPrev() {
  if(viewOrder.empty())
    return NULL;
  nextTab++;
  if(nextTab == viewOrder.end()) {
    nextTab = viewOrder.begin();
  }
  return *nextTab;
}

bool FlatTabCtrl::hasDirtyTabs() const {
  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    if ((*i)->dirty) {
      return true;
    }
  }
  return false;
}

void FlatTabCtrl::setActive(HWND aWnd) {
  if (!inTab) {
    setTop(aWnd);
  }
  TabInfo* ti = getTabInfo(aWnd);
  dcassert(ti != NULL);
  active = ti;
  if (ti->dirty) {
    ti->dirty = false;
    if (!hasDirtyTabs()) {
      KillTimer(DIRTY_FLASH_TIMER_ID);
    }
  }
  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    updateTabToolRect(*i);
  }
  Invalidate();
}

void FlatTabCtrl::setTop(HWND aWnd) {
  viewOrder.remove(aWnd);
  viewOrder.push_back(aWnd);
  nextTab = --viewOrder.end();
}

void FlatTabCtrl::setDirty(HWND aWnd) {
  if (TabInfo* ti = getTabInfo(aWnd)) {
    bool inval = ti->update();
    if (inval) {
      updateTabToolText(ti);
    }
    if (active != ti) {
      if (!ti->dirty) {
        if (!hasDirtyTabs()) {
          m_dirtyFlashOn = true;
          SetTimer(DIRTY_FLASH_TIMER_ID, DIRTY_FLASH_TIMER_INTERVAL);
        }
	ti->dirty = true;
	inval = true;
      }
    }
    if (inval) {
      Invalidate();
    }
  }
}

void FlatTabCtrl::setIconState(HWND aWnd) {
  if(TabInfo* ti = getTabInfo(aWnd)) {
    ti->bState = true;
    ti->hCustomIcon = NULL; // !SMT!-UI
    Invalidate();
  }
}

void FlatTabCtrl::unsetIconState(HWND aWnd) {
  TabInfo* ti = getTabInfo(aWnd);
  if(ti != NULL) {
    ti->bState = false;
    ti->hCustomIcon = NULL; // !SMT!-UI
    Invalidate();
  }
}

// !SMT!-UI
void FlatTabCtrl::setCustomIcon(HWND aWnd, HICON custom) {
  TabInfo* ti = getTabInfo(aWnd);
  if(ti != NULL) {
    ti->hCustomIcon = custom;
    Invalidate();
  }
}

#if 0
void FlatTabCtrl::setColor(HWND aWnd, COLORREF color) {
  TabInfo* ti = getTabInfo(aWnd);
  if(ti != NULL) {
    ti->pen.DeleteObject();
    ti->pen.CreatePen(PS_SOLID, 1, color);
    Invalidate();
  }
}
#endif

void FlatTabCtrl::updateText(HWND aWnd, LPCTSTR text) {
  TabInfo* ti = getTabInfo(aWnd);
  if (ti != NULL) {
    ti->updateText(text);
    updateTabToolText(ti, text);
    Invalidate();
  }
}

LRESULT FlatTabCtrl::onLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
  const int x = GET_X_LPARAM(lParam);
  const int y = GET_Y_LPARAM(lParam);
  const TabInfo* t = findTab(x, y);
  if (t) {
    if (isCloseHover(t, x, y) || wParam & MK_SHIFT) {
      ::SendMessage(t->hWnd, WM_CLOSE, 0, 0);
    }
    else {
      moving = const_cast<TabInfo*>(t);
    }
  }
  return 0;
}

LRESULT FlatTabCtrl::onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  if (moving) {
    int x = GET_X_LPARAM(lParam);
    const TabInfo* t = findTab(x, GET_Y_LPARAM(lParam));
    if (t) {	
      if (t == moving) {
        HWND hWnd = GetParent();
        if (hWnd) {
          ::SendMessage(hWnd, FTM_SELECTED, (WPARAM) t->hWnd, 0);
        }
      }
      else {
        // check if the pointer is on the left or right half of the tab to determine where to insert the tab
        moveTabs(t, x > t->xpos + t->getWidth() / 2);
      }
    }
    else {
      moveTabs(tabs.back(), true);
    }
    moving = NULL;
  }
  return 0;
}

LRESULT FlatTabCtrl::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);
  POINT pt = { x, y };        // location of mouse click 
  ScreenToClient(&pt); 
  const TabInfo* t = findTab(pt.x, pt.y);
  if (t) {
    if (!::SendMessage(t->hWnd, FTM_CONTEXTMENU, 0, lParam)) {
      closing = t->hWnd;
      OMenu contextMenu;
      contextMenu.CreatePopupMenu();
      contextMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
      contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_TOPALIGN, x, y, m_hWnd);
    }
  }
  return 0;
}

LRESULT FlatTabCtrl::onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);
  const TabInfo* t = findTab(x, y);
  if (t) {
    if (isCloseHover(t, x, y)) {
      if (t != closeHoverTab) {
        closeHoverTab = const_cast<TabInfo*>(t);
        Invalidate();
      }
      return 0;
    }
  }
  if (closeHoverTab != NULL) {
    closeHoverTab = NULL;
    Invalidate();
  }
  return 0;
}

bool FlatTabCtrl::isCloseHover(const TabInfo* tab, int x, int y) const {
  CRect rect;
  getTabRect(tab, rect);
  return x >= rect.right - CLOSE_BUTTON_WIDTH - CLOSE_BUTTON_RIGHT_MARGIN
    && x < rect.right - CLOSE_BUTTON_RIGHT_MARGIN 
    && y >= rect.top + CLOSE_BUTTON_TOP_MARGIN 
    && y < rect.top + CLOSE_BUTTON_TOP_MARGIN + CLOSE_BUTTON_HEIGHT;
}

void FlatTabCtrl::getTabRect(const TabInfo* tab, LPRECT rect) const {
  if (tab->row < 0) {
    memzero(rect, sizeof(RECT));
  }
  else {
    int row = tab->row;
    if (active && active->row >= 0) {
      if (active->row != rows - 1) {
        row += rows - 1 - active->row;
        row %= rows;
      }
    }
    rect->top = topMargin + row * (getTabHeight() + vertGap);
    if (active && active->row == tab->row) {
      rect->top += activeTabDelta;
    }
    rect->left = tab->xpos;
    rect->right = tab->xpos + tab->width;
    rect->bottom = rect->top + getTabHeight();
  }
}

LRESULT FlatTabCtrl::onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if(::IsWindow(closing))
    ::SendMessage(closing, WM_CLOSE, 0, 0);
  return 0;
}

int FlatTabCtrl::getMaxRightPos() const {
  CRect clientRect;
  GetClientRect(clientRect);
  return clientRect.Width() - rightMargin;
}

void FlatTabCtrl::calcRows(bool inval) {
#ifdef _DEBUG
  if (tab_tip.IsWindow()) {
    dcassert((size_t) tab_tip.GetToolCount() == tabs.size());
  }
#endif
  const int maxRight = getMaxRightPos();
  const int maxRows = SETTING(MAX_TAB_ROWS);
  int r = 0;
  int w = leftMargin;
  bool notify = false;
  bool needInval = false;  
  const int extraWidth = ICON_LEFT_MARGIN + ICON_WIDTH + CLOSE_BUTTON_WIDTH + CLOSE_BUTTON_RIGHT_MARGIN + 2;
  __dcdebug("maxRight=%d\n", maxRight);
  int tabWidth = tabs.size() > 0 ? (maxRight - leftMargin + horzGap) / tabs.size() - horzGap : 0;
  __dcdebug("initial tabWidth=%d\n", tabWidth);
  tabWidth = max(tabWidth, charWidth * MINIMAL_TAB_WIDTH + extraWidth);
  tabWidth = min(tabWidth, charWidth * STANDARD_TAB_WIDTH + extraWidth);
  __dcdebug("final tabWidth=%d\n", tabWidth);

  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    TabInfo* ti = *i;
    ti->width = tabWidth;
    __dcdebug(" w=%d\n", w);
    if (r >= 0 && w + ti->getWidth() > maxRight) {
      ++r;
      if (r >= maxRows) {
        notify |= (rows != r);
	rows = r;
	r = -1;
	chevron.ShowWindow(SW_SHOW);
      } 
      else {
	w = leftMargin;
      }
    } 
    needInval |= (ti->row != r);
    ti->xpos = w;
    ti->row = r;
    w += horzGap;
    w += ti->getWidth();
  }
  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    updateTabToolRect(*i);
  }
  if (r >= 0) {
    chevron.ShowWindow(SW_HIDE);
    ++r;
    notify |= (rows != r);
    rows = r;
  }
  if (notify) {
    ::SendMessage(GetParent(), FTM_ROWS_CHANGED, 0, 0);
  }
  if (needInval && inval) {
    Invalidate();
  }
}

void FlatTabCtrl::addTabTool(const TabInfo* tab) {
  CRect rect;
  getTabRect(tab, rect);
  const int len = ::GetWindowTextLength(tab->hWnd) + 1;
  AutoArray<TCHAR> text(len);
  ::GetWindowText(tab->hWnd, text, len);
  CToolInfo ti(TTF_SUBCLASS, m_hWnd, (UINT_PTR) tab, rect, text);
  ControlAdjuster::fixToolInfoSize(ti);
#ifdef _DEBUG
  int count = tab_tip.GetToolCount();
#endif
  tab_tip.AddTool(&ti);
#ifdef _DEBUG
  dcassert(tab_tip.GetToolCount() == count + 1);
#endif
}

void FlatTabCtrl::deleteTabTool(const TabInfo* tab) {
#ifdef _DEBUG
  int count = tab_tip.GetToolCount();
#endif
  CToolInfo ti(0, m_hWnd, (UINT_PTR) tab, NULL, NULL);
  ControlAdjuster::fixToolInfoSize(ti);
  tab_tip.DelTool(ti);
#ifdef _DEBUG
  dcassert(tab_tip.GetToolCount() == count - 1);
#endif
}

void FlatTabCtrl::updateTabToolText(const TabInfo* tab, LPCTSTR tabText) {
  const int len = (tabText != NULL ? _tcsclen(tabText) : ::GetWindowTextLength(tab->hWnd)) + 1;
  AutoArray<TCHAR> text(len);
  if (tabText != NULL) {
    _tcscpy((LPTSTR) text, tabText);
  }
  else {
    ::GetWindowText(tab->hWnd, (LPTSTR) text, len);
  }
  __dcdebug("updateTabToolText=%ws\n", text);
  CToolInfo ti(0, m_hWnd, (UINT_PTR) tab, NULL, (LPTSTR) text);
  ControlAdjuster::fixToolInfoSize(ti);
  tab_tip.UpdateTipText(&ti);
}

void FlatTabCtrl::updateTabToolRect(const TabInfo* tab) {
  CRect rect;
  getTabRect(tab, rect);
  CToolInfo ti(0, m_hWnd, (UINT_PTR) tab, &rect, NULL);
  ControlAdjuster::fixToolInfoSize(ti);
  tab_tip.SetToolRect(ti);
}

LRESULT FlatTabCtrl::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
/*
Кнопка chevron используется для вызова меню табов, для которых не хватило места 
для отображения из-за того, что ограничение количество строк. 
Ограничение задается на странице "Для экспертов", параметр MAX_TAB_ROWS
*/
  chevron.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    BS_PUSHBUTTON , 0, IDC_CHEVRON);
  chevron.SetWindowText(_T("\u00bb"));

  mnu.CreatePopupMenu();

  tab_tip.Create(m_hWnd, rcDefault, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX);

  CClientDC dc(m_hWnd);
  const HFONT oldFont = dc.SelectFont(WinUtil::font);
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  tabHeight = max(LARGE_ICON_HEIGHT + 2 * ICON_TOP_MARGIN, (int) tm.tmHeight + 2);
  charWidth = tm.tmAveCharWidth;
  dc.SelectFont(oldFont);
  return 0;
}

LRESULT FlatTabCtrl::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) { 
  calcRows();
  SIZE sz = { LOWORD(lParam), HIWORD(lParam) };
  chevron.MoveWindow(sz.cx-14, 1, 14, 14);
  return 0;
}

LRESULT FlatTabCtrl::onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  return 1;
}

COLORREF selectInactiveTabColor(COLORREF color, int delta, int divisor) {
  int r = GetRValue(color);
  int g = GetGValue(color);
  int b = GetBValue(color);
  int maxPartValue = 255 * divisor / (divisor + delta);
  if (r < maxPartValue && g < maxPartValue && b < maxPartValue) {
    r = min(255, r * (divisor + delta) / divisor);
    g = min(255, g * (divisor + delta) / divisor);
    b = min(255, b * (divisor + delta) / divisor);
  }
  else {
    r = r * (divisor - delta) / divisor;
    g = g * (divisor - delta) / divisor;
    b = b * (divisor - delta) / divisor;
  }
  return RGB(r, g, b);
}

COLORREF selectPenColor(COLORREF color, int delta, int divisor) {
  int r = GetRValue(color);
  int g = GetGValue(color);
  int b = GetBValue(color);
  r = r * (divisor - delta) / divisor;
  g = g * (divisor - delta) / divisor;
  b = b * (divisor - delta) / divisor;
  return RGB(r, g, b);
}

LRESULT FlatTabCtrl::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect clientRect;
  GetClientRect(clientRect);
  CBrush background;
  background.CreateSysColorBrush(COLOR_WINDOW);
  dc.FillRect(&clientRect, background);
  CBrush tabColor;
  tabColor.CreateSysColorBrush(COLOR_BTNFACE);
  CBrush inactiveTabColor;
  COLORREF btnFaceColor = GetSysColor(COLOR_BTNFACE);
  inactiveTabColor.CreateSolidBrush(selectInactiveTabColor(btnFaceColor, 1, 10));
  const HBRUSH oldBrush = dc.SelectBrush(inactiveTabColor);
  const HFONT oldFont = dc.SelectFont(WinUtil::font);
  CPen inactivePen;
  inactivePen.CreatePen(PS_SOLID, 1, btnFaceColor);
  CPen activePen;
  activePen.CreatePen(PS_SOLID, 1, selectPenColor(btnFaceColor, 2, 10));
  const HPEN oldPen = dc.SelectPen(inactivePen);
  dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
  typedef vector<TabInfo*>* TabInfoPtrVector;
  typedef vector<TabInfo*>::iterator TabInfoIterator;
  AutoArray<TabInfoPtrVector> tabsByRows(rows);
  for (int i = 0; i < rows; ++i) {
    // TODO эти объекты нужно удалять при выходе.
    tabsByRows[i] = new vector<TabInfo*>();
  }
  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    TabInfo* t = *i;	
    if (t && t->row >= 0 && t->row < rows) {
      tabsByRows[t->row]->push_back(t);
    }
  }
  for (int i = 0; i < rows; ++i) {
    TabInfoPtrVector v = tabsByRows[i];
    sort(v->begin(), v->end(), compareTabInfoByPosX);
  }  
  for (int i = 0; i < rows; ++i) {
    TabInfoPtrVector v = tabsByRows[i];
    TabInfoIterator activeFindResult = find(v->begin(), v->end(), active);
    if (activeFindResult != v->end()) {
      if (i != rows - 1) {
        int delta = rows - i - 1;
        for (int j = 0; j < delta; ++j) {
          TabInfoPtrVector temp = tabsByRows[j];
          int next = j;
          while ((next += delta) < rows) {
            swap(temp, tabsByRows[next]);
          }
          tabsByRows[j] = temp;
        }
      }
      break;
    }
  }
  for (int r = 0; r < rows; ++r) {
    int top = topMargin + r * (getTabHeight() + vertGap);
    if (r == rows - 1) {
      top += activeTabDelta;
    }
    const TabInfoPtrVector v = tabsByRows[r];
    for (TabInfo::ListIter i = v->begin(); i != v->end(); ++i) {
      const TabInfo* t = *i;
      if (t == active) {
        dc.SelectBrush(tabColor);
        dc.SelectPen(activePen);
        drawTab((HDC)dc, t, top, true);
        dc.SelectBrush(inactiveTabColor);
        dc.SelectPen(inactivePen);
      }
      else {
        drawTab((HDC)dc, t, top, false);
      }
    }
  }
  if (bottomMargin > 1) {
    clientRect.top = clientRect.bottom - (bottomMargin - 1);
    dc.FillRect(clientRect, tabColor);
  }
  dc.SelectPen(oldPen);
  dc.SelectFont(oldFont);
  dc.SelectBrush(oldBrush);
  return 0;
}

LRESULT FlatTabCtrl::onChevron(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  while (mnu.GetMenuItemCount() > 0) {
    mnu.RemoveMenu(0, MF_BYPOSITION);
  }
  int n = 0;
  CRect rc;
  GetClientRect(&rc);
  CMenuItemInfo mi;
  mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA | MIIM_STATE;
  mi.fType = MFT_STRING | MFT_RADIOCHECK;

  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    TabInfo* ti = *i;
    if (ti->row == -1) {
      mi.dwTypeData = (LPTSTR) ti->name;
      mi.dwItemData = (ULONG_PTR) ti->hWnd;
      mi.fState = MFS_ENABLED | (ti->dirty ? MFS_CHECKED : 0);
      mi.wID = IDC_SELECT_WINDOW + n;
      mnu.InsertMenuItem(n++, TRUE, &mi);
    } 
  }

  POINT pt;
  chevron.GetClientRect(&rc);
  pt.x = rc.right - rc.left;
  pt.y = 0;
  chevron.ClientToScreen(&pt);

  mnu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
  return 0;
}

LRESULT FlatTabCtrl::onSelectWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  CMenuItemInfo mi;
  mi.fMask = MIIM_DATA;

  mnu.GetMenuItemInfo(wID, FALSE, &mi);
  HWND hWnd = GetParent();
  if(hWnd) {
    SendMessage(hWnd, FTM_SELECTED, (WPARAM)mi.dwItemData, 0);
  }
  return 0;		
}

void FlatTabCtrl::SwitchTo(bool next) {  
  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i){
    FlatTabCtrl::TabInfo* t = *i;
    if (t->hWnd == active->hWnd) {
      if (next) {
	++i;
        if (i == tabs.end()) {
	  i = tabs.begin();
        }
      } 
      else {
        if (i == tabs.begin()) {
	  i = tabs.end();
        }
	--i;
      }
      t = *i;
      setActive(t->hWnd);
      ::SetWindowPos(t->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
      break;
    }
  }	
}

LRESULT FlatTabCtrl::onCloseTab(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  TabInfo* tab = findTab(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  if (tab) {
    ::SendMessage(tab->hWnd, WM_CLOSE, 0, 0);
  }
  return 0;
}

void FlatTabCtrl::moveTabs(const TabInfo* aTab, bool after){
  if (moving == NULL) {
    return;
  }
  //remove the tab we're moving
  for(TabInfo::List::iterator j = tabs.begin(); j != tabs.end(); ++j){
    if((*j) == moving){
      tabs.erase(j);
      break;
    }
  }
  //find the tab we're going to insert before or after
  TabInfo::List::iterator i;
  for (i = tabs.begin(); i != tabs.end(); ++i) {
    if ((*i) == aTab) {
      if (after) {
	++i;
      }
      break;
    }
  }
  tabs.insert(i, moving);
  moving = NULL;
  calcRows(false);
  Invalidate();	
}

FlatTabCtrl::TabInfo* FlatTabCtrl::getTabInfo(HWND aWnd) const {
  for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
    FlatTabCtrl::TabInfo* t = *i;
    if (t->hWnd == aWnd) {
      return t;
    }
  }
  return NULL;
}

FlatTabCtrl::TabInfo* FlatTabCtrl::findTab(int x, int y) const {
  for (int r = 0; r < rows; ++r) {
    int row = r;
    if (active && active->row >= 0) {
      if (active->row != rows - 1) {
        row += rows - 1 - active->row;
        row %= rows;
      }
    }
    int top = topMargin + row * (getTabHeight() + vertGap);
    if (row == rows - 1) {
      top += activeTabDelta;
    }
    if (y >= top && y < top + getTabHeight()) {
      /* нашли строку */
      //dcdebug("find r=%d (row=%d)\n", r, row);
      for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
        TabInfo* t = *i;
        if (t->row == r && x >= t->xpos && x < t->xpos + t->getWidth()) {
          /* нашли элемент */
          return t;
        }
      }
      /* элемент в строке не найден */
      return NULL;
    }
  }
  /* не найдена даже подходящая строка */
  return NULL;
}

void FlatTabCtrl::drawTab(CDCHandle dc, const TabInfo* tab, int ypos, bool aActive) {
  CRect r;
  r.top = ypos;
  if (aActive) {
    r.top -= activeTabDelta;
  }
  r.left = tab->xpos;
  r.right = tab->xpos + tab->getWidth();
  r.bottom = ypos + getTabHeight();
  if (aActive && bottomMargin > 0) {
    CRect clientRect;
    GetClientRect(clientRect);
    dc.MoveTo(clientRect.left, r.bottom);
    dc.LineTo(r.left, r.bottom);
    dc.LineTo(r.left, r.top);
    dc.LineTo(r.right - 1, r.top);
    dc.LineTo(r.right - 1, r.bottom);
    dc.LineTo(clientRect.right, r.bottom);
  }
  else {
    dc.MoveTo(r.left, r.bottom - 1);
    dc.LineTo(r.left, r.top);
    dc.LineTo(r.right - 1, r.top);
    dc.LineTo(r.right - 1, r.bottom);
  }
  ++r.top;
  ++r.left;
  --r.right;
  if (aActive && bottomMargin > 0) {
    ++r.bottom;
  }
  dc.FillRect(r, dc.GetCurrentBrush());
  dc.SetBkMode(TRANSPARENT);
  if (aActive) {
    dc.SelectFont(WinUtil::boldFont);
  } 
  else {
    dc.SelectFont(WinUtil::font);
  }
  HBITMAP bmpClose = tab == closeHoverTab ? bmpCloseHover : (aActive ? bmpCloseActive : bmpCloseInactive);
  if (bmpClose) {
    CDC comp;
    comp.CreateCompatibleDC(dc);    
    comp.SelectBitmap(bmpClose);
    dc.BitBlt(
      r.right - CLOSE_BUTTON_WIDTH - CLOSE_BUTTON_RIGHT_MARGIN, 
      r.top + CLOSE_BUTTON_TOP_MARGIN, 
      CLOSE_BUTTON_WIDTH, 
      CLOSE_BUTTON_HEIGHT, 
      comp, 
      0, 
      0, 
      SRCCOPY);
  }
  HICON hIcon;
  if (tab->hCustomIcon) {
    hIcon = tab->hCustomIcon;
  }
  else {
    hIcon = getIconByIndex(tab->bState && tab->m_altIconIndex ? tab->m_altIconIndex : tab->m_iconIndex);
  }
  if (hIcon) {
    if (tab->m_largeIcon) {
      DrawIconEx(
        dc, 
        r.left + ICON_LEFT_MARGIN, 
        r.top + ICON_TOP_MARGIN, 
        hIcon, 
        LARGE_ICON_WIDTH, 
        LARGE_ICON_HEIGHT, 
        NULL, 
        NULL, 
        DI_NORMAL);
    }
    else {
      DrawIconEx(
        dc, 
        r.left + ICON_LEFT_MARGIN, 
        r.top + ICON_TOP_MARGIN, 
        hIcon, 
        ICON_WIDTH, 
        ICON_HEIGHT, 
        NULL, 
        NULL, 
        DI_NORMAL);
    }
  }
  if (m_dirtyFlashOn && tab->dirty) {
    icoTabDirty.DrawIconEx(
      dc, 
      r.right - (CLOSE_BUTTON_WIDTH + CLOSE_BUTTON_RIGHT_MARGIN),
      r.bottom - ICON_HEIGHT,
      ICON_WIDTH,
      ICON_HEIGHT,
      NULL,
      NULL,
      DI_NORMAL | DI_COMPAT);
  }
  r.left += (tab->m_largeIcon ? LARGE_ICON_WIDTH : ICON_WIDTH) + ICON_LEFT_MARGIN + 1;
  r.right -= CLOSE_BUTTON_WIDTH + CLOSE_BUTTON_RIGHT_MARGIN + 1;
  dc.DrawText(tab->name, tab->len, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

LRESULT FlatTabCtrl::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (hasDirtyTabs()) {
    m_dirtyFlashOn = !m_dirtyFlashOn;
    for (TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
      TabInfo* ti = *i;
      if (ti->dirty) {
        RECT r;
        getTabRect(ti, &r);
        if (!IsRectEmpty(&r)) {
          r.top = r.bottom - ICON_HEIGHT;
          r.left = r.right - (CLOSE_BUTTON_WIDTH + CLOSE_BUTTON_RIGHT_MARGIN);
          r.right = r.left + ICON_WIDTH;
          InvalidateRect(&r);
        }
      }
    }    
  }
  else {
    KillTimer(DIRTY_FLASH_TIMER_ID);
  }
  return 0;
}

FlatTabCtrl::TabInfo::TabInfo(MDIContainer::Window window, bool largeIcon, int icon, int stateIcon):
m_window(window),
hWnd(window->m_hWnd),
len(0), 
xpos(0), 
width(100),
row(0), 
dirty(false), 
m_largeIcon(largeIcon),
m_iconIndex(icon), 
m_altIconIndex(stateIcon), 
hCustomIcon(NULL), 
bState(false) 
{
  name[0] = 0;
  update();
}

FlatTabCtrl::TabInfo::~TabInfo() {
}

bool FlatTabCtrl::TabInfo::update() {
  TCHAR name2[MAX_LENGTH];
  len = (size_t)::GetWindowTextLength(hWnd);
  if (len >= MAX_LENGTH) {
    ::GetWindowText(hWnd, name2, MAX_LENGTH - 3);
    name2[MAX_LENGTH - 4] = _T('.');
    name2[MAX_LENGTH - 3] = _T('.');
    name2[MAX_LENGTH - 2] = _T('.');
    name2[MAX_LENGTH - 1] = 0;
    len = MAX_LENGTH - 1;
  }
  else {
    ::GetWindowText(hWnd, name2, MAX_LENGTH);
  }
  if (_tcscmp(name, name2) == 0) {
    return false;
  }
  _tcscpy(name, name2);
  return true;
}

bool FlatTabCtrl::TabInfo::updateText(const LPCTSTR text) {
  len = _tcslen(text);
  if (len >= MAX_LENGTH) {
    ::_tcsncpy(name, text, MAX_LENGTH - 3);
    name[MAX_LENGTH - 4] = '.';
    name[MAX_LENGTH - 3] = '.';
    name[MAX_LENGTH - 2] = '.';
    name[MAX_LENGTH - 1] = 0;
    len = MAX_LENGTH - 1;
  }
  else {
    _tcscpy(name, text);
  }
  return true;
}

/**
 * @file
 * $Id: FlatTabCtrl.cpp,v 1.19 2008/03/10 07:42:28 alexey Exp $
 */
