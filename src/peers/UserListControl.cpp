#include "stdafx.h"
#include "UserListControl.h"
#include "CaptionFont.h"
#include "ControlAdjuster.h"
#include "../client/IgnoreManager.h"

#define FILTER_LENGTH 20
#define FILTER_RIGHT_MARGIN 8
#define FILTER_CAPTION_GAP 8
#define HEADER_FILTER_GAP 8

#define ICON_WIDTH     32
#define ICON_HEIGHT    32
#define ICON_RIGHT_GAP  8

const LV::Column UserListControl::defaultColumns[] = {
	{ COLUMN_NICK, 125, ResourceManager::USER_LIST_NICK, true, LV::Column::LEFT },
	{ COLUMN_SHARED, 100, ResourceManager::SHARED, true, LV::Column::RIGHT },
	{ COLUMN_EXACT_SHARED, 75, ResourceManager::EXACT_SHARED, false, LV::Column::RIGHT },
	{ COLUMN_DESCRIPTION, 125, ResourceManager::DESCRIPTION, true, LV::Column::LEFT },
	{ COLUMN_TAG, 100, ResourceManager::TAG, false, LV::Column::LEFT },
	{ COLUMN_EMAIL, 125, ResourceManager::USER_LIST_CONTACTS, true, LV::Column::LEFT },
	{ COLUMN_HUBS, 40, ResourceManager::HUBS, false, LV::Column::LEFT },
	{ COLUMN_SLOTS, 40, ResourceManager::SLOTS, false, LV::Column::RIGHT },
	{ COLUMN_IP, 40, ResourceManager::IP_BARE, false, LV::Column::RIGHT },
#ifdef PPA_INCLUDE_DNS
	{ COLUMN_DNS, 100, ResourceManager::DNS_BARE, false, LV::Column::LEFT }
#endif
};

static int userListDblClickCommands[] = {
  IDC_GETLIST,
  IDC_PUBLIC_MESSAGE,
  IDC_PRIVATEMESSAGE,
  IDC_MATCH_QUEUE,
  IDC_GRANTSLOT,
  IDC_ADD_TO_FAVORITES
};

UserListControl::UserListControl(UserListControlListener* listener)
: 
sorted(true),
invalidateCount(false),
invalidateBegin(0),
invalidateEnd(0),
m_listener(listener), 
m_copyMenu(this), 
ctrlUsers(this) {
}

LRESULT UserListControl::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE, IDC_USER_LIST_FILTER);
  ctrlFilter.SetFont(WinUtil::font);
  ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA, WS_EX_CLIENTEDGE, IDC_USERS);
  ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
  ctrlUsers.SetFont(WinUtil::font);
  m_headerHeight = ControlAdjuster::adjustHeaderHeight(m_hWnd);
  return 0;
}

LRESULT UserListControl::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CRect r;
  GetClientRect(r);
  if (ctrlUsers) {
    CRect rectUserList(r);
    rectUserList.top += m_headerHeight;
    ctrlUsers.MoveWindow(rectUserList);
    ctrlUsers.Invalidate();
  }
  if (ctrlFilter) {
    const SIZE filterSize = ControlAdjuster::adjustEditSize(ctrlFilter, FILTER_LENGTH);
    ctrlFilter.MoveWindow(r.right - FILTER_RIGHT_MARGIN - filterSize.cx, 
                         (m_headerHeight - filterSize.cy) / 2, 
                         filterSize.cx, 
                         filterSize.cy);
  }
  return 0;
}

LRESULT UserListControl::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  r.bottom = m_headerHeight;
  dc.FillRect(r, GetSysColorBrush(COLOR_BTNFACE));
  dc.SetBkMode(TRANSPARENT);
  tstring title = TSTRING(USER_LIST_TITLE);
  const CaptionFont font(CaptionFont::BOLD);
  const HFONT oldFont = dc.SelectFont(font);
  CIcon headerIcon;
  headerIcon.LoadIcon(IDI_USER_LIST_HEADER);
  headerIcon.DrawIcon(dc, 0, (m_headerHeight - ICON_HEIGHT) / 2);
  r.left += ICON_WIDTH + ICON_RIGHT_GAP;
  CRect drawRect(r);
  dc.DrawText(title.c_str(), title.length(), drawRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
  if (ctrlFilter) {
	  CRect rectFilter;
	  ctrlFilter.GetWindowRect(rectFilter);
	  ScreenToClient(rectFilter);
	  if (r.left + drawRect.Width() + HEADER_FILTER_GAP > rectFilter.left) {
		  title = _T("ѕользователи");
		  dc.DrawText(title.c_str(), title.length(), drawRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
	  }
  }
  dc.DrawText(title.c_str(), title.length(), r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  r.left += drawRect.Width();
  r.left += HEADER_FILTER_GAP;
  if (ctrlFilter) {
    CRect rectFilter;
    ctrlFilter.GetWindowRect(rectFilter);
    ScreenToClient(rectFilter);
    if (r.left < rectFilter.left) {
      drawRect = r;
      const tstring filterCaption = TSTRING(USER_LIST_FILTER_CAPTION);
      dc.SelectFont(WinUtil::font);
      dc.DrawText(filterCaption.c_str(), filterCaption.length(), drawRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
      if (r.left + drawRect.Width() < rectFilter.left) {
        r.left = max(r.left, rectFilter.left - drawRect.Width() - FILTER_CAPTION_GAP);
        dc.DrawText(filterCaption.c_str(), filterCaption.length(), r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
      }
    }
  }
  dc.SelectFont(oldFont);
  return 0;
}

void UserListControl::initColumns(const tstring& server) {
	ctrlUsers.InitColumns(defaultColumns);
#if 0
  int columnIndexes[COLUMN_LAST];
  int columnSizes[COLUMN_LAST];
  memcpy(columnIndexes, defaultColumnIndexes, COLUMN_LAST * sizeof(int));
  memcpy(columnSizes, defaultColumnSizes, COLUMN_LAST * sizeof(int));

  const FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
  if (fhe) {
    WinUtil::splitTokens(columnIndexes, fhe->getHeaderOrder(), COLUMN_LAST);
    WinUtil::splitTokens(columnSizes, fhe->getHeaderWidths(), COLUMN_LAST);
  }
  else {
    WinUtil::splitTokens(columnIndexes, SETTING(HUBFRAME_ORDER), COLUMN_LAST);
    WinUtil::splitTokens(columnSizes, SETTING(HUBFRAME_WIDTHS), COLUMN_LAST);
  }

  for(uint8_t j=0; j<COLUMN_LAST; j++) {
    int fmt = (j == COLUMN_SHARED || j == COLUMN_EXACT_SHARED  || j == COLUMN_SLOTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
    ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
  }

  ctrlUsers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
  string columnVisibility;
  if (fhe) {
    columnVisibility = fhe->getHeaderVisible();
  } 
  if (columnVisibility.empty()) {
    columnVisibility = SETTING(HUBFRAME_VISIBLE);
  }
  ctrlUsers.setVisible(columnVisibility);
#endif

  ctrlUsers.SetBkColor(WinUtil::bgColor);
  ctrlUsers.SetTextBkColor(WinUtil::bgColor);
  ctrlUsers.SetTextColor(WinUtil::textColor);
  ctrlUsers.setFlickerFree(WinUtil::bgBrush);

  //[?] PPA
  ctrlUsers.setSortColumn(COLUMN_NICK);

  ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);
}

FavoriteHubEntry* UserListControl::saveColumns(const tstring& server) {
#if 0
  string order, widths, visible;
  ctrlUsers.saveHeaderOrder(order, widths, visible);
#endif
  FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
  if (fhe != NULL) {
#if 0
    fhe->setHeaderOrder(order);
    fhe->setHeaderWidths(widths);
    fhe->setHeaderVisible(visible);
#endif
    return fhe;
  }
  else {
#if 0
    SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_ORDER, order);
    SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_WIDTHS, widths);
    SettingsManager::getInstance()->set(SettingsManager::HUBFRAME_VISIBLE, visible);
#endif
    return NULL;
  }
}

void UserListControl::locateInList(UserInfo* ui) {
  const size_t items = m_users.size();
  int pos = -1;
  ctrlUsers.SetRedraw(FALSE);
  for (size_t i = 0; i < items; ++i) {
    int state;
    if (m_users[i] == ui) {
      pos = i;
      state = LVIS_SELECTED | LVIS_FOCUSED;
    }
    else {
      state = 0;
    }
    ctrlUsers.SetItemState(i, state, LVIS_SELECTED | LVIS_FOCUSED);
  }
  ctrlUsers.SetRedraw(TRUE);
  if (pos >= 0) {
    ctrlUsers.EnsureVisible(pos, FALSE);
  }
}

void UserListControl::locateInList(const tstring& nick) {
	const int pos = ctrlUsers.findItem(nick);
	if (pos >= 0) {
		size_t items = m_users.size();
		ctrlUsers.SetRedraw(FALSE);
		for (size_t i = 0; i < items; ++i) {
			int state = (i == (size_t) pos) ? LVIS_SELECTED | LVIS_FOCUSED : 0;
			ctrlUsers.SetItemState(i, state, LVIS_SELECTED | LVIS_FOCUSED);
		}
		ctrlUsers.SetRedraw(TRUE);
		ctrlUsers.EnsureVisible(pos, FALSE);
	}
}

/* 
ищет пользователе€ по префиксу, возвращает найденное им€ или пустую строку 
если не найдено. если пользователь найден, то он выдел€етс€ в списке 
*/
tstring UserListControl::autoCompleteUserNick(const tstring& prefix) {
  dcassert(prefix.length() > 0);
  const int focused = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED);
  const int offset = max(focused, 0) + 1;
  const size_t itemCount = m_users.size();
  for (size_t i = 0; i < itemCount; ++i) {
    const size_t index = (i + offset) % itemCount;
    const UserInfo* ui = m_users[index];
    const tstring& nick = ui->getText(COLUMN_NICK);
    bool found = Util::strnicmp(nick, prefix, prefix.length()) == 0;
    if (!found) {
      // Check if there's one or more [ISP] tags to ignore...
      tstring::size_type y = 0;
      while (nick[y] == _T('[')) {
        tstring::size_type x = nick.find(_T(']'), y);
        if (x == string::npos) {
          break;
        }
        if (Util::strnicmp(nick.c_str() + x + 1, prefix.c_str(), prefix.length()) == 0) {
          found = true;
          break;
        }
        y = x + 1; // assuming that nick[y] == '\0' is legal
      }
    }
    if (found) {
      if (focused >= 0) {
        ctrlUsers.SetItemState(focused, 0, LVNI_SELECTED | LVNI_FOCUSED);
      }
      ctrlUsers.SetItemState(index, LVNI_FOCUSED | LVNI_SELECTED, LVNI_FOCUSED | LVNI_SELECTED);
      ctrlUsers.EnsureVisible(index, FALSE);
      return nick;
    }
  }
  return Util::emptyStringT;
}

void UserListControl::updateColors() {
  ctrlUsers.SetImageList(WinUtil::userImages, LVSIL_SMALL);
  ctrlUsers.Invalidate();
  if (ctrlUsers.GetBkColor() != WinUtil::bgColor) {
    ctrlUsers.SetBkColor(WinUtil::bgColor);
    ctrlUsers.SetTextBkColor(WinUtil::bgColor);
    ctrlUsers.setFlickerFree(WinUtil::bgBrush);
  }
  if (ctrlUsers.GetTextColor() != WinUtil::textColor) {
    ctrlUsers.SetTextColor(WinUtil::textColor);
  }
}

LRESULT UserListControl::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
  aSelectedUser = UserPtr(NULL);  // !SMT!-S
  POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
  CRect rc;            // client area of window
  ctrlUsers.GetHeader().GetWindowRect(&rc);
  if (PtInRect(&rc, pt)) {
    ctrlUsers.showMenu(pt);
    return 0;
  }
  if (reinterpret_cast<HWND>(wParam) == ctrlUsers) { 
    if (ctrlUsers.GetSelectedCount() == 1) {
      if (pt.x == -1 && pt.y == -1) { // вызов меню с клавиатуры
        WinUtil::getContextMenuPos(ctrlUsers, pt);
      }
      int i = -1;
      i = ctrlUsers.GetNextItem(i, LVNI_SELECTED);
      if ( i >= 0 ) {
        aSelectedUser = getItem(i)->getUser(); // !SMT!-S
      }
    }
    CMenu Mnu;
    Mnu.CreatePopupMenu();
    const bool bIsMe = (aSelectedUser == ClientManager::getInstance()->getMe());
    if (!bIsMe) {
      Mnu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
      Mnu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
	  if (aSelectedUser) {
		  Mnu.AppendMenu(MF_SEPARATOR);
		  if (!IgnoreManager::getInstance()->isIgnored(aSelectedUser)) {
			  Mnu.AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
		  } 
		  else {    
			  Mnu.AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
		  }
	  }
      Mnu.AppendMenu(MF_SEPARATOR);
      Mnu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
      Mnu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU)); // !SMT!-UI
      Mnu.AppendMenu(MF_SEPARATOR);
    }
    Mnu.AppendMenu(MF_POPUP, (UINT) m_copyMenu.build(), CTSTRING(COPY));
    Mnu.AppendMenu(MF_STRING, IDC_REFRESH, CTSTRING(REFRESH_USER_LIST));
    if (ctrlUsers.GetSelectedCount() > 0) {
      Mnu.SetMenuDefaultItem(getDoubleClickAction());
      Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
    }
    WinUtil::UnlinkStaticMenus(Mnu); // !SMT!-UI
  }
  return 0;
}

LRESULT UserListControl::onDoubleClickUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
  NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;
  if (item->iItem != -1) {
    UserInfo* ui = const_cast<UserInfo*>(m_users[item->iItem]);
    if (ui && m_listener) {
      m_listener->onDoubleClickUser(this, ui);
    }
  }
  return 0;
}

LRESULT UserListControl::onEnterUsers(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
  int item = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED);
  if (item != -1) {
    try {
      QueueManager::getInstance()->addList(m_users[item]->getUser(), QueueItem::FLAG_CLIENT_VIEW);
    }
    catch(const Exception& e) {
      if (m_listener) {
        m_listener->userListMessage(Text::toT(e.getError()));
      }
    }
  }
  return 0;
}

void UserListControl::reloadUserList() {
	m_users.clear();
	const UserInfo::UserMap& userMap = m_listener->getUserMap();
	if (!isFilterActive()) {
		for (UserInfo::UserMapIter i = userMap.begin(); i != userMap.end(); ++i){
			UserInfo* ui = i->second;
			if (!ui->isHidden()) {
				m_users.push_back(ui);
			}
		}
	}
	else {
		const UserFilter* filter = getCurrentFilter();
		for (UserInfo::UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
			UserInfo* ui = i->second;
			if (!ui->isHidden() && filter->match(ui)) {
				m_users.push_back(ui);
			}
		}
	}
	resortList();
	ctrlUsers.SetItemCountEx(m_users.size(), 0);
}

void UserListControl::addItem(UserInfo* ui) {
	if (!ui->isHidden()) {
		if (!isFilterActive() || getCurrentFilter()->match(ui)) {
			if (!sorted) {
				m_users.push_back(ui);
				countChanged();
				redrawItems(m_users.size() - 1, m_users.size());
			}
			else {
				size_t index = m_users.add(ui);
				countChanged();
				redrawItems(index, m_users.size());
			}
		}
	}
}

void UserListControl::updateItem(UserInfo* ui) {
	if (ui->isHidden() || isFilterActive() && !getCurrentFilter()->match(ui)) {
		deleteItem(ui);
	}
	else {
		size_t pos = m_users.indexOf(ui);
		if (pos != UserListContainer::npos) {
			UserListContainer::ValidationStatus status = m_users.isOrderValid(pos, ui);
			if (status == UserListContainer::vsOK) {
				redrawItems(pos, pos + 1);
			}
			else {
				size_t newPos;
				if (status == UserListContainer::vsPrevGreater) {
					newPos = m_users.getInsertPos(ui, 0, pos);
				}
				else {
					dcassert(status == UserListContainer::vsNextSmaller);
					newPos = m_users.getInsertPos(ui, pos + 1, m_users.size() - pos - 1);
				}
				dcassert(pos != newPos);
				m_users.move(pos, newPos);
				redrawItems(min(pos,newPos), max(pos,newPos)+1);
			}
		}
		else {
			if (!sorted) {
				m_users.push_back(ui);
				countChanged();
				redrawItems(m_users.size() - 1, m_users.size());
			}
			else {
				pos = m_users.add(ui);
				countChanged();
				redrawItems(pos, m_users.size());
			}
		}
	}
}

void UserListControl::deleteItem(UserInfo* ui) {
	size_t pos = m_users.remove(ui);
	if (pos != UserListContainer::npos) {
		countChanged();
		if (pos < m_users.size()) {			
			redrawItems(pos, m_users.size());
		}
	}
}

LRESULT UserListControl::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
  LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
  switch(cd->nmcd.dwDrawStage) 
  {
  case CDDS_PREPAINT:
    return CDRF_NOTIFYITEMDRAW;
  case CDDS_ITEMPREPAINT:
    onCustomDrawUserItem(m_users[cd->nmcd.dwItemSpec], cd);
    return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
  default:
    return CDRF_DODEFAULT;
  }
}

void UserListControl::onCustomDrawUserItem(const UserInfo* ui, LPNMLVCUSTOMDRAW cd) {
  Client* client = m_listener != NULL ? m_listener->getClient() : NULL;
  WinUtil::getUserColor(ui->getUser(), cd->clrText, cd->clrTextBk, client); 
  if (client->isOp()) {
    if (ui->getIdentity().isBC()) {
      cd->clrText = SETTING(BAD_CLIENT_COLOUR);
    } 
    else if (ui->getIdentity().isBF()) {
      cd->clrText = SETTING(BAD_FILELIST_COLOUR);
    } 
    else if (BOOLSETTING(SHOW_SHARE_CHECKED_USERS)) {
      bool cClient = ui->getIdentity().isTC();
      bool cFilelist = ui->getIdentity().isFC();
      if (cClient && cFilelist) {
        cd->clrText = SETTING(FULL_CHECKED_COLOUR);
      } 
      else if (cClient) {
        cd->clrText = SETTING(CLIENT_CHECKED_COLOUR);
      } 
      else if (cFilelist) {
        cd->clrText = SETTING(FILELIST_CHECKED_COLOUR);
      }
    }
  }
}

LRESULT UserListControl::onFilterChange(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
  const int filterTextLength = ctrlFilter.GetWindowTextLength();
  if (filterTextLength > 0) {
    ctrlFilter.showClearButton();
  }
  else {
    ctrlFilter.hideClearButton();
  }
  AutoArray<TCHAR> buf(filterTextLength + 1);
  ctrlFilter.GetWindowText(buf, filterTextLength + 1);
  m_filter = buf;
  reloadUserList();
  return 0;
}

class StringUserFilter: public UserFilter {
private:
  const string m_filter;
public:
  StringUserFilter(const string& filter):m_filter(filter) { }

  virtual bool match(const UserInfo* ui) const {
    return Util::findSubString(ui->getNick(), m_filter) != string::npos;
  }
};

UserFilter* UserListControl::getCurrentFilter() { 
  if (m_filter.empty()) {
    return new UserFilter(); 
  }
  else {
    return new StringUserFilter(Text::fromT(m_filter));
  }
}

int UserListControl::decodeDoubleClickAction(int action) const {
  if (action >= 0 && action < COUNTOF(userListDblClickCommands)) {
    return userListDblClickCommands[action];
  }
  else {
    return -1;
  }
}

int UserListControl::getDoubleClickAction() const {
  return decodeDoubleClickAction(SETTING(USERLIST_DBLCLICK));
}

// Sorting
LRESULT UserListControl::onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if (l->iSubItem != ctrlUsers.getSortColumn()) {
		ctrlUsers.setAscending(true);
		ctrlUsers.setSortColumn(l->iSubItem);
	} else if (ctrlUsers.isAscending()) {
		ctrlUsers.setAscending(false);
	} else {
		ctrlUsers.setSortColumn(-1);
	}
	UserInfoComparator* comparator = NULL;
	switch (ctrlUsers.getRealSortColumn())
	{
	case COLUMN_NICK:   
		comparator = new UserInfoNickComparator(); break;
	case COLUMN_SHARED:
	case COLUMN_EXACT_SHARED:
		comparator = new UserInfoShareComparator(); break;
	case COLUMN_DESCRIPTION:
		comparator = new UserInfoDescriptionComparator(); break;
	case COLUMN_TAG:
		comparator = new UserInfoTagComparator(); break;
	case COLUMN_EMAIL:
		comparator = new UserInfoEmailComparator(); break;
	case COLUMN_HUBS:
		comparator = new UserInfoHubsComparator(); break;
	case COLUMN_SLOTS:
		comparator = new UserInfoSlotsComparator(); break;
	case COLUMN_IP:
		comparator = new UserInfoIpComparator(); break;
	}
	if (comparator == NULL) {
		comparator = new UserInfoNickComparator();
	}
	if (!ctrlUsers.isAscending()) {
		comparator = new UserInfoReverseComparator(comparator);
	}
	m_users.setComparator(comparator);
	if (m_users.size() > 0) {
		resortList();
		ctrlUsers.RedrawItems(0, m_users.size() - 1);
	}
	return 0;
}

void UserListControl::beginUpdate() {
	// ctrlUsers.SetRedraw(FALSE);
	saveSelection();
}

void UserListControl::endUpdate() {
	if (!sorted) {
		resortList();
		sorted = true;
	}
	// ctrlUsers.SetRedraw(TRUE);
	if (invalidateCount) {
		ctrlUsers.SetItemCountEx(
			m_users.size(), 
			(invalidateBegin < invalidateEnd ? LVSICF_NOINVALIDATEALL : 0 ) | LVSICF_NOSCROLL
		);
		invalidateCount = false;
	}
	if (invalidateBegin < invalidateEnd) {
		ctrlUsers.RedrawItems(invalidateBegin, min(invalidateEnd, m_users.size()) - 1);
		invalidateBegin = 0;
		invalidateEnd = 0;
	}
	for (vector<const UserInfo*>::iterator i = selectedObjects.begin(); i != selectedObjects.end(); ++i) {
		size_t index = m_users.indexOf(*i);
		if (index != UserListContainer::npos) {
			if (selectedIndexes.erase(index) == 0) {
				if (*i == focused) {
					ctrlUsers.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				}
				else {
					ctrlUsers.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
	}
	for (set<size_t>::iterator i = selectedIndexes.begin(); i != selectedIndexes.end(); ++i) {
		ctrlUsers.SetItemState(*i, 0, LVIS_SELECTED);
	}
}
