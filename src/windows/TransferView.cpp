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

#include "../client/QueueManager.h"
#include "../client/QueueItem.h"

#include "TransferView.h"
#include "MainFrm.h"
#include "UsersFrame.h" // !SMT!-S
#include "../peers/PrivateFrameFactory.h"

#include "BarShader.h"
#include "../peers/CaptionFont.h"
#include "../peers/Sounds.h"

ListColumn TransferView::columns[COLUMN_LAST] = {
  { COLUMN_USER, 150, ResourceManager::TRANSFER_VIEW_COLUMN_USER, true },
  { COLUMN_FILE, 175, ResourceManager::FILENAME, true },
  { COLUMN_SIZE, 100, ResourceManager::SIZE, true, ListColumn::RIGHT },
  { COLUMN_STATUS, 250, ResourceManager::TRANSFER_VIEW_COLUMN_STATUS, true },
  { COLUMN_TIMELEFT, 75, ResourceManager::TIME_LEFT, true, ListColumn::RIGHT },
  { COLUMN_SPEED, 75, ResourceManager::SPEED, true, ListColumn::RIGHT },
  { COLUMN_PATH, 200, ResourceManager::PATH, true },
  { COLUMN_HUB, 100, ResourceManager::HUB_SEGMENTS, false },
  { COLUMN_LOCATION, 50, ResourceManager::LOCATION_BARE, false },
  { COLUMN_IP, 50, ResourceManager::IP_BARE, false },
#ifdef PPA_INCLUDE_DNS
  { COLUMN_DNS, 75, ResourceManager::DNS_BARE, false },
#endif
  { COLUMN_SHARE, 100, ResourceManager::SHARED, false },
  { COLUMN_SLOTS, 75, ResourceManager::SLOTS, false }
};

ListColumnSettings TransferView::listSettings = {
  SettingsManager::MAINFRAME_ORDER,
  SettingsManager::MAINFRAME_WIDTHS,
  SettingsManager::MAINFRAME_VISIBLE
};

TransferView::TransferView(): 
PreviewAppsSize(0), 
m_headerHeight(0),
m_constructed(false),
m_mouseClicked(false),
m_visible(false)
{
  m_mouseClickPoint.x = -1;
  m_mouseClickPoint.y = -1;
}

TransferView::~TransferView() {
        OperaColors::ClearCache();
}

static COLORREF selectViewColor(COLORREF color, int delta, int divisor) {
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

LRESULT TransferView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_headerHeight = MulDiv(16, HIWORD(GetDialogBaseUnits()), 8);
  CRect r;
  GetClientRect(r);
  selectToggleButtonLocation(r);
  toggleButton.Create(m_hWnd, r, CTSTRING(TRANSFER_VIEW_BUTTON_HIDE), IDC_TRANSFER_VIEW_TOGGLE);
  toggleButton.setAlignment(FlatButton::taCenter);
  toggleButton.setHighLightColor(HOVER_COLOR);
  toggleButton.setBackgroundColor(selectViewColor(GetSysColor(COLOR_BTNFACE), 1, 10));
  toggleButton.SetFont(WinUtil::font);

	arrows.CreateFromImage(IDB_ARROWS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	user.LoadIcon(IDR_TUSER, 16, 16, LR_DEFAULTSIZE);
	speedImages.CreateFromImage(IDB_TSPEEDS, 16, 5, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	speedImagesBW.CreateFromImage(IDB_TSPEEDSBW, 16, 5, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);
	ctrlTransfers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | LVS_EX_INFOTIP);

        ctrlTransfers.InitColumns(columns, listSettings);
	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	ctrlTransfers.setFlickerFree(WinUtil::bgBrush);

	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	ctrlTransfers.setSortColumn(COLUMN_USER);

	transferMenu.CreatePopupMenu();
	appendUserItems(transferMenu);
        transferMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
        transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::speedMenu, CTSTRING(UPLOAD_SPEED_LIMIT)); // !SMT!-S
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CTSTRING(FORCE_ATTEMPT));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));

        // !SMT!-UI
        copyMenu.CreatePopupMenu();
        copyMenu.InsertSeparatorFirst(TSTRING(COPY));
        for(int i = 0; i < COLUMN_LAST; ++i)
                copyMenu.AppendMenu(MF_STRING, IDC_COPY + i, CTSTRING_I(columns[i].name));
        copyMenu.AppendMenu(MF_SEPARATOR);
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_WMLINK, CTSTRING(COPY_MLINK_TEMPL));

	previewMenu.CreatePopupMenu();
	previewMenu.AppendMenu(MF_SEPARATOR);
	usercmdsMenu.CreatePopupMenu();

	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	transferMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)usercmdsMenu, CTSTRING(SETTINGS_USER_COMMANDS));
        transferMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY)); // !SMT!-UI
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
	transferMenu.AppendMenu(MF_SEPARATOR);
	transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(CLOSE_CONNECTION));
	transferMenu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);

	segmentedMenu.CreatePopupMenu();
        segmentedMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
	segmentedMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	segmentedMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));
	segmentedMenu.AppendMenu(MF_STRING, IDC_MENU_SLOWDISCONNECT, CTSTRING(SETCZDC_DISCONNECTING_ENABLE));
        segmentedMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY)); // !SMT!-UI
        segmentedMenu.AppendMenu(MF_STRING, IDC_ASK_SLOT, CTSTRING(ASK_SLOT)); // !SMT!-UI
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_CONNECT_ALL, CTSTRING(CONNECT_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_DISCONNECT_ALL, CTSTRING(DISCONNECT_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_EXPAND_ALL, CTSTRING(EXPAND_ALL));
	segmentedMenu.AppendMenu(MF_STRING, IDC_COLLAPSE_ALL, CTSTRING(COLLAPSE_ALL));
	segmentedMenu.AppendMenu(MF_SEPARATOR);
	segmentedMenu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_ALL));

	ConnectionManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	return 0;
}

void TransferView::prepareClose() {
  ctrlTransfers.saveHeaderOrder(listSettings);
  ConnectionManager::getInstance()->removeListener(this);
  DownloadManager::getInstance()->removeListener(this);
  UploadManager::getInstance()->removeListener(this);
  SettingsManager::getInstance()->removeListener(this);
  WinUtil::UnlinkStaticMenus(transferMenu); // !SMT!-UI
}

LRESULT TransferView::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CRect rc;
  GetClientRect(rc);
  bool newVisible = rc.Height() > m_headerHeight;
  if (newVisible != m_visible) {
    m_visible = newVisible;
    toggleButton.SetWindowText(newVisible ? CTSTRING(TRANSFER_VIEW_BUTTON_HIDE) : CTSTRING(TRANSFER_VIEW_BUTTON_SHOW));
    if (m_constructed) { 
      MainFrame::getMainFrame()->UISetCheck(ID_VIEW_TRANSFER_VIEW, newVisible);
      SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, newVisible);
    }
  }
  CRect buttonLocation(rc); 
  selectToggleButtonLocation(buttonLocation);
  toggleButton.MoveWindow(buttonLocation);
  rc.top += m_headerHeight;
  const int hGap = GetSystemMetrics(SM_CXSIZEFRAME);
  rc.left += hGap;
  rc.right -= hGap;
  ctrlTransfers.MoveWindow(rc);
  return 0;
}

//на входе clientrect - на выходе координаты кнопки
void TransferView::selectToggleButtonLocation(LPRECT rect) {
  const int buttonHeight = m_headerHeight * 2 / 3;
  const int delta = (m_headerHeight - buttonHeight) / 2;
  rect->top += delta;
  rect->bottom = rect->top + buttonHeight;
  rect->right -= delta;
  rect->left = rect->right - 4 * buttonHeight;
  __dcdebug("toggleButtonLocation %d %d\n", rect->left, rect->top);
}

LRESULT TransferView::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  CBrush brush;
  brush.CreateSolidBrush(selectViewColor(GetSysColor(COLOR_BTNFACE), 1, 10));
  dc.FillRect(r, brush);
  r.bottom = r.top + m_headerHeight;
#ifdef _DEBUG_
  CBrush b;
  b.CreateSolidBrush(RGB(255,0,0));
  dc.FillRect(r, b);
#endif
  const tstring title = TSTRING(TRANSFER_VIEW_TITLE);
  dc.SetBkMode(TRANSPARENT);
  CaptionFont font;
  HFONT oldFont = dc.SelectFont(font);
  CRect drawRect(r);
  dc.DrawText(title.c_str(), title.length(), drawRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
  r.left += max(0, r.Height() - drawRect.Height()) / 2;
  dc.DrawText(title.c_str(), title.length(), r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  dc.SelectFont(oldFont);
  return 0;
}

void TransferView::setVisibility(bool value) {
  MainFrame* mainFrame = MainFrame::getMainFrame();
  CRect splitRect;
  mainFrame->GetSplitterRect(splitRect);
  if (value) {
    const int range = splitRect.Height() - mainFrame->m_cxySplitBar - mainFrame->m_cxyBarEdge;
    int newPos = MulDiv(m_proportionalPos, range, mainFrame->m_nPropMax);
    if (newPos < getHeaderHeight() || range - newPos <= 2 * getHeaderHeight()) {
      newPos = range - 3 * getHeaderHeight();
    }
    mainFrame->SetSplitterPos(newPos);
    if (!m_visible) {
      CRect r2;
      mainFrame->m_footer.GetSplitterRect(r2);
      mainFrame->m_footer.SetSplitterPos(r2.Height());
    }
  }
  else {
    const int newPos = splitRect.Height() - mainFrame->m_cxySplitBar - mainFrame->m_cxyBarEdge;
    mainFrame->SetSplitterPos(newPos);
  }
}

LRESULT TransferView::onToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  setVisibility(!isVisible());
  return 0;
}

LRESULT TransferView::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlTransfers && ctrlTransfers.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTransfers, pt);
		}
		
		if(ctrlTransfers.GetSelectedCount() > 0) {
			int i = -1;
			bool bCustomMenu = false;
			ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
			bool main = ii->subItems.size() > 1;

                        updateSummary(); // !SMT!-UI

			if(!main && (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				ItemInfo* itemI = ctrlTransfers.getItemData(i);
				bCustomMenu = true;
	
				usercmdsMenu.InsertSeparatorFirst(TSTRING(SETTINGS_USER_COMMANDS));
	
                                // !SMT!-S
                                selectedUser = itemI->user;
                                if(selectedUser != (UserPtr)NULL) {
                                        prepareMenu(usercmdsMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(selectedUser->getCID()));
                                        // !SMT!-S
                                        for (int j = 0; j < WinUtil::speedMenu.GetMenuItemCount(); j++)
                                           WinUtil::speedMenu.CheckMenuItem(j, MF_BYPOSITION | MF_UNCHECKED);
                                        FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
                                        FavoriteManager::FavoriteMap::const_iterator ui = favUsers.find(selectedUser->getCID());
                                        if (ui != favUsers.end()) {
                                           const FavoriteUser & u = ui->second;
                                           int id = IDC_SPEED_NONE;
                                           switch (u.getUploadLimit()) {
                                              case FavoriteUser::UL_SU:  id = IDC_SPEED_SUPER; break;
                                              case FavoriteUser::UL_BAN: id = IDC_SPEED_BAN; break;
                                              case FavoriteUser::UL_2:   id = IDC_SPEED_02K; break;
                                              case FavoriteUser::UL_5:   id = IDC_SPEED_05K; break;
                                              case FavoriteUser::UL_8:   id = IDC_SPEED_08K; break;
                                              case FavoriteUser::UL_12:  id = IDC_SPEED_12K; break;
                                              case FavoriteUser::UL_16:  id = IDC_SPEED_16K; break;
                                              case FavoriteUser::UL_24:  id = IDC_SPEED_24K; break;
                                              case FavoriteUser::UL_32:  id = IDC_SPEED_32K; break;
                                              case FavoriteUser::UL_64:  id = IDC_SPEED_64K; break;
                                              case FavoriteUser::UL_128: id = IDC_SPEED_128K; break;
                                              case FavoriteUser::UL_256: id = IDC_SPEED_256K; break;
                                           }
                                           WinUtil::speedMenu.CheckMenuItem(id, MF_BYCOMMAND | MF_CHECKED);
                                        }
                                }
                                // end !SMT!-S
			}
			WinUtil::ClearPreviewMenu(previewMenu);

#ifdef PPA_INCLUDE_DROP_SLOW
			segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
			transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_UNCHECKED);
#endif
			if(ii->download) {
                                transferMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_ENABLED);
#ifdef PPA_INCLUDE_DROP_SLOW
				transferMenu.EnableMenuItem(IDC_MENU_SLOWDISCONNECT, MFS_ENABLED);
#endif
				if(!ii->Target.empty()) {
					string target = Text::fromT(ii->Target);
					string ext = Util::getFileExt(target);
					if(ext.size()>1) ext = ext.substr(1);
					PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);

					QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

					QueueItem::StringIter qi = queue.find(&target);
#ifdef PPA_INCLUDE_DROP_SLOW
					bool slowDisconnect = false;
					if(qi != queue.end())
						slowDisconnect = qi->second->isSet(QueueItem::FLAG_AUTODROP);
#endif
					QueueManager::getInstance()->unlockQueue();
#ifdef PPA_INCLUDE_DROP_SLOW
					if(slowDisconnect) {
						segmentedMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
						transferMenu.CheckMenuItem(IDC_MENU_SLOWDISCONNECT, MF_BYCOMMAND | MF_CHECKED);
					}
#endif
				}
			} else {
                                transferMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MFS_DISABLED);
#ifdef PPA_INCLUDE_DROP_SLOW
				transferMenu.EnableMenuItem(IDC_MENU_SLOWDISCONNECT, MFS_DISABLED);
#endif
			}

			previewMenu.InsertSeparatorFirst(TSTRING(PREVIEW_MENU));
				
                        if (!main) {
                          checkAdcItems(transferMenu);
                          transferMenu.EnableMenuItem(IDC_PRIORITY_PAUSED, ii->download && ii->status == ItemInfo::STATUS_RUNNING ? MFS_ENABLED : MFS_DISABLED);
                          transferMenu.EnableMenuItem((UINT)(HMENU)previewMenu, previewMenu.GetMenuItemCount() > 1 ? MFS_ENABLED : MFS_DISABLED);
                          transferMenu.EnableMenuItem((UINT)(HMENU)usercmdsMenu, usercmdsMenu.GetMenuItemCount() > 1 ? MFS_ENABLED : MFS_DISABLED);
                          transferMenu.InsertSeparatorFirst(TSTRING(MENU_TRANSFERS));
                          transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
                          transferMenu.RemoveFirstItem();
                        }
                        else {
                          segmentedMenu.EnableMenuItem(IDC_PRIORITY_PAUSED, ii->download && ii->status == ItemInfo::STATUS_RUNNING ? MFS_ENABLED : MFS_DISABLED);
                          segmentedMenu.EnableMenuItem((UINT)(HMENU)previewMenu, previewMenu.GetMenuItemCount() > 1 ? MFS_ENABLED : MFS_DISABLED);
                          segmentedMenu.EnableMenuItem((UINT)(HMENU)usercmdsMenu, usercmdsMenu.GetMenuItemCount() > 1 ? MFS_ENABLED : MFS_DISABLED);
                          segmentedMenu.InsertSeparatorFirst(TSTRING(SETTINGS_SEGMENT));
                          segmentedMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
                          segmentedMenu.RemoveFirstItem();
                        }

			if ( bCustomMenu ) {
				WinUtil::ClearPreviewMenu(usercmdsMenu);
			}
			return TRUE; 
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

// !SMT!-UI
static bool _vector_find(const vector<uint64_t>& v, uint64_t value) {
    for (vector<uint64_t>::const_iterator i = v.begin(); i != v.end(); ++i)
        if ((*i) == value)
            return true;
    return false;
}
// !SMT!-UI
LRESULT TransferView::onAskSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        ItemInfo* ii = ctrlTransfers.getItemData(ctrlTransfers.GetNextItem(-1, LVNI_SELECTED));
        vector<uint64_t> sentShare;
        StringMap params;
        string filename = Text::fromT(Util::getFileName(ii->Target));
        params["filename"] = filename;
        string::size_type point = filename.find_last_of('.');
        params["shortname"] = (point != string::npos && point > 0)? filename.substr(0, point) : filename;
        for(ItemInfo::Iter i = ii->subItems.begin(); i != ii->subItems.end(); i++) {

                const UserPtr& u = (*i)->getUser();
                 const uint64_t l_Share = u->getBytesShared();

                if (_vector_find(sentShare, l_Share)) // don't send to same users(on other hubs)
                        continue;
                sentShare.push_back(l_Share);
                // don't ask from user with running download
                if (DownloadManager::getInstance()->checkFileDownload(u))
                        continue;

                params["nick"] = u->getFirstNick();
                PrivateFrameFactory::openWindow(u, Text::toT(Util::formatParams(SETTING(SLOT_ASK), params, false)));
        }
        return 0;
}

// !SMT!-UI. todo: move same code to template CopyBaseHandler
LRESULT TransferView::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        tstring data;
        int i = -1, columnId = wID-IDC_COPY; // !SMT!-UI: copy several rows
        while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
                ItemInfo* ii = ctrlTransfers.getItemData(i);
                tstring sdata;

                TTHValue tth;
                QueueManager::getInstance()->getTTH(Text::fromT(ii->Target), tth);

                if (wID == IDC_COPY_LINK)
                        sdata = Text::toT(WinUtil::getMagnet(tth, Util::getFileName(ii->Target), ii->size));
                else if (wID == IDC_COPY_WMLINK)
                        sdata = Text::toT(WinUtil::getWebMagnet(tth, Util::getFileName(ii->Target), ii->size));
                else
                        sdata = ii->getText(columnId);

                if (data.empty())
                        data = sdata;
                else
                        data = data + L"\r\n" + sdata;
        }
        WinUtil::setClipboard(data);
        return 0;
}

void TransferView::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* itemI = ctrlTransfers.getItemData(i);
		if(!itemI->user->isOnline())
			continue;

		StringMap tmp = ucParams;
		ucParams["fileFN"] = Text::fromT(itemI->Target);

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		
		ClientManager::getInstance()->userCommand(itemI->user, uc, tmp, true);
	}
}

LRESULT TransferView::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  for (Selection i(this); i.hasNext(); ) {
    ItemInfo *ii = i.next();	
    ii->columns[COLUMN_STATUS] = CTSTRING(CONNECTING_FORCED);
    ctrlTransfers.updateItem( ii );
    ClientManager::getInstance()->connect(ii->user);
  }
  return 0;
}

void TransferView::ItemInfo::removeAll() {
	if(subItems.size() <= 1) {
		QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	} else {
		if(!BOOLSETTING(CONFIRM_DELETE) || ::MessageBox(0, _T("Do you really want to remove this item?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			QueueManager::getInstance()->remove(Text::fromT(Target));
	}
}

LRESULT TransferView::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
   if( g_RunningUnderWine) //[+]PPA
   {
	bHandled = FALSE;
	return 0;
   }

	CRect rc;
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:{
		ItemInfo* ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		int colIndex = ctrlTransfers.findColumn(cd->iSubItem);
		cd->clrTextBk = WinUtil::bgColor;


     if( BOOLSETTING(USE_CUSTOM_LIST_BACKGROUND) && cd->nmcd.dwItemSpec % 2 != 0) 
        {
			cd->clrTextBk = HLS_TRANSFORM(cd->clrTextBk, -9, 0);
		}

		if((ii->status == ItemInfo::STATUS_RUNNING) && (colIndex == COLUMN_STATUS) ) {
                        if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
				bHandled = FALSE;
				return 0;
			}

			// Get the color of this bar
			COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
				(ii->download ? (!ii->main ? SETTING(DOWNLOAD_BAR_COLOR) : SETTING(PROGRESS_SEGMENT_COLOR)) : SETTING(UPLOAD_BAR_COLOR)) : 
				GetSysColor(COLOR_HIGHLIGHT);
			if(!ii->download && BOOLSETTING(UP_TRANSFER_COLORS)) //[+]PPA
			{	 
			const int l_NumSlot = ii->getUser()->getCountSlots();
			if(l_NumSlot != 0)
			{
				if (l_NumSlot < 5) 
			 	clr = 0; 
			 else
               if (l_NumSlot < 10) //[+]PPA
                 clr = 0x00AEAEAE;
			}
			else
              clr = 0x00FFD7FF; 
			}
			//this is just severely broken, msdn says GetSubItemRect requires a one based index
			//but it wont work and index 0 gives the rect of the whole item
			if(cd->iSubItem == 0) {
				//use LVIR_LABEL to exclude the icon area since we will be painting over that
				//later
				ctrlTransfers.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
			} else {
				ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
			}

			/* Thanks & credits for Stealthy style go to phaedrus */
			bool useStealthyStyle = BOOLSETTING(STEALTHY_STYLE);
			//bool isMain = (!ii->main || !ii->download);
			//bool isSmaller = (!ii->main && ii->collapsed == false);

			// fixes issues with double border
			if(useStealthyStyle) {
				rc.top -= 1;
				//if(isSmaller)
					//rc.bottom -= 1;
			}

			// Real rc, the original one.
			CRect real_rc = rc;
			// We need to offset the current rc to (0, 0) to paint on the New dc
			rc.MoveToXY(0, 0);
			
			CRect rc4;
			CRect rc2;
			rc2 = rc;

			// Text rect
			if(useStealthyStyle){
				rc2.left += 23; // indented for icon and text
				rc2.right -= 2; // and without messing with the border of the cell
				rc2.top -= 1; // and fix text vertical alignment
				// Background rect
				rc4 = rc;
			}else{
				rc2 = rc;
				rc2.left += 6; // indented with 6 pixels
				rc2.right -= 2; // and without messing with the border of the cell
				// Background rect
				rc4 = rc;
				rc2.left += 9;
			}

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());

			HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
			HDC& dc = cdc.m_hDC;
			
			COLORREF barPal[3] = { HLS_TRANSFORM(clr, -40, 50), clr, HLS_TRANSFORM(clr, 20, -30) };
			COLORREF barPal2[3] = { HLS_TRANSFORM(clr, -15, 0), clr, HLS_TRANSFORM(clr, 15, 0) };
			COLORREF oldcol;
			// The value throws off, usually with about 8-11 (usually negatively f.ex. in src use 190, the change might actually happen already at aprox 180)
			HLSCOLOR hls = RGB2HLS(clr);
			LONG top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2 + 1;
			
			HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
			SetBkMode(dc, TRANSPARENT);

			// Get the color of this text bar - this way it ends up looking nice imo.
			if(!useStealthyStyle) {
			oldcol = ::SetTextColor(dc, SETTING(PROGRESS_OVERRIDE_COLORS2) ? 
				(ii->download ? SETTING(PROGRESS_TEXT_COLOR_DOWN) : SETTING(PROGRESS_TEXT_COLOR_UP)) : 
				OperaColors::TextFromBackground(clr));
			} else {
				if(clr == RGB(255, 255, 255)) // see if user is using white as clr, rare but you may never know
					oldcol = ::SetTextColor(dc, RGB(0, 0, 0));
				else
					oldcol = ::SetTextColor(dc, barPal2[1]);
			}

			// Draw the background and border of the bar	
			if(ii->size == 0) ii->size = 1;		
		
			if(BOOLSETTING(PROGRESSBAR_ODC_STYLE) || useStealthyStyle) {
				// New style progressbar tweaks the current colors
				HLSTRIPLE hls_bk = OperaColors::RGB2HLS(cd->clrTextBk);

				// Create pen (ie outline border of the cell)
				HPEN penBorder = ::CreatePen(PS_SOLID, 1, OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.6 : 0.4));
				HGDIOBJ pOldPen = ::SelectObject(dc, penBorder);

				// Draw the outline (but NOT the background) using pen
				HBRUSH hBrOldBg = CreateSolidBrush(cd->clrTextBk);
				hBrOldBg = (HBRUSH)::SelectObject(dc, hBrOldBg);

				if(useStealthyStyle)
					::Rectangle(dc, rc4.left, rc4.top, rc4.right, rc4.bottom);
				else
					::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
				
				DeleteObject(::SelectObject(dc, hBrOldBg));

				// Set the background color, by slightly changing it
				HBRUSH hBrDefBg = CreateSolidBrush(OperaColors::blendColors(cd->clrTextBk, clr, (hls_bk.hlstLightness > 0.75) ? 0.85 : 0.70));
				HGDIOBJ oldBg = ::SelectObject(dc, hBrDefBg);

				// Draw the outline AND the background using pen+brush
				if(useStealthyStyle)
					::Rectangle(dc, rc4.left, rc4.top, rc4.left + (LONG)(rc4.Width() * ii->getRatio() + 0.5), rc4.bottom);
				else
					::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * ii->getRatio() + 0.5), rc.bottom);

				if(useStealthyStyle) {
					// Draw the text over entire item
					::ExtTextOut(dc, rc2.left, top, ETO_CLIPPED, rc2, ii->getText(COLUMN_STATUS).c_str(), ii->getText(COLUMN_STATUS).length(), NULL);

					rc.right = rc.left + (int) (((int64_t)rc.Width()) * ii->actual / ii->size);
				
					if(ii->pos != 0)
						rc.bottom -= 1;

					rc.top += 1;
				
					//create bar pen
					if(HLS_S(hls) <= 30) // good values would be 20-30
						penBorder = ::CreatePen(PS_SOLID, 1, barPal2[0]);
					else
						penBorder = ::CreatePen(PS_SOLID, 1, barPal[0]);
				
					DeleteObject(::SelectObject(dc, penBorder));

					//create bar brush
					hBrDefBg = CreateSolidBrush(barPal[1]);
				
					DeleteObject(::SelectObject(dc, hBrDefBg));
				
					//draw bar
					::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);

					//draw bar highlight
					if(rc.Width() > 4){
						DeleteObject(SelectObject(cdc, CreatePen(PS_SOLID,1,barPal[2])));
						rc.top += 2;
						::MoveToEx(cdc,rc.left+2,rc.top,(LPPOINT)NULL);
						::LineTo(cdc,rc.right-2,rc.top);
					}
				}
				// Reset pen
				DeleteObject(::SelectObject(dc, pOldPen));
				// Reset bg (brush)
				DeleteObject(::SelectObject(dc, oldBg));
			}

                        // Draw the background and border of the bar
			if(!BOOLSETTING(PROGRESSBAR_ODC_STYLE) && !useStealthyStyle) {
				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), ii->size);

				rc.right = rc.left + (int) (rc.Width() * ii->pos / ii->size); 
				if(!ii->download) {
					statusBar.FillRange(0, ii->start, HLS_TRANSFORM(clr, -20, 30));
					statusBar.FillRange(ii->start, ii->actual,  clr);
				} else {
					statusBar.FillRange(0, ii->actual, clr);
					if(ii->main)
						statusBar.FillRange(ii->start, ii->actual, SETTING(PROGRESS_SEGMENT_COLOR));
				}
				if(ii->pos > ii->actual)
					statusBar.FillRange(ii->actual, ii->pos, SETTING(PROGRESS_COMPRESS_COLOR));

				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));
			} else {
				if(!useStealthyStyle) {
					int right = rc.left + (int) ((int64_t)rc.Width() * ii->actual / ii->size);
                
					COLORREF a, b;
					OperaColors::EnlightenFlood(clr, a, b);
					OperaColors::FloodFill(cdc, rc.left+1, rc.top+1, right, rc.bottom-1, a, b, BOOLSETTING(PROGRESSBAR_ODC_BUMPED));
				}
			}

			if(useStealthyStyle) {
				// Draw icon - Nasty way to do the filelist icon, but couldn't get other ways to work well, TODO: do separating filelists from other transfers the proper way...
				if(ii->getText(COLUMN_PATH).find(Text::toT(Util::getListPath())) != string::npos || ii->getText(COLUMN_PATH).find(Text::toT(Util::getConfigPath())) != string::npos) {
					DrawIconEx(dc, rc2.left - 20, rc2.top + 2, user, 16, 16, NULL, NULL, DI_NORMAL | DI_COMPAT);
				} else if(ii->status == ItemInfo::STATUS_RUNNING) {
					RECT rc9 = rc2;
					rc9.left -= 20;
					rc9.top += 4;
					rc9.right = rc9.left + 16;
					rc9.bottom = rc9.top + 12;

					int64_t speedkb = ii->speed / 1000;
					int64_t speedmark;
					if(!BOOLSETTING(THROTTLE_ENABLE)) {
						if(!ii->download) {
							speedmark = SETTING(TOP_UP_SPEED) / 5;
						} else {
							speedmark = SETTING(TOP_SPEED) / 5;
						}
					} else {
						if(!ii->download) {
								speedmark = SETTING(MAX_UPLOAD_SPEED_LIMIT) / 5;
						} else {
								speedmark = SETTING(MAX_DOWNLOAD_SPEED_LIMIT) / 5;
						}
					}

					if((HLS_S(hls) > 30) || (HLS_L(hls) < 70)) {
						if(speedkb >= speedmark * 5)
							speedImages.DrawEx(4, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else if(speedkb >= speedmark * 4)
							speedImages.DrawEx(3, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else if(speedkb >= speedmark * 3)
							speedImages.DrawEx(2, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else if(speedkb >= speedmark * 2)
							speedImages.DrawEx(1, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else
							speedImages.DrawEx(0, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
					} else { // color can be assumed to be a shade of grey, use greyscale speedImages
						if(speedkb >= speedmark * 5)
							speedImagesBW.DrawEx(4, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else if(speedkb >= speedmark * 4)
							speedImagesBW.DrawEx(3, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else if(speedkb >= speedmark * 3)
							speedImagesBW.DrawEx(2, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else if(speedkb >= speedmark * 2)
							speedImagesBW.DrawEx(1, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
						else
							speedImagesBW.DrawEx(0, dc, rc9, CLR_DEFAULT, CLR_DEFAULT, ILD_IMAGE);
					}
				}
				// use white to as many colors as possible (values might need some tweaking), I didn't like TextFromBackground...
				if(((HLS_L(hls) > 190) && (HLS_S(hls) <= 30)) || (HLS_L(hls) > 211))
					oldcol = ::SetTextColor(dc, HLS_TRANSFORM(clr, -40, 0));
				else
					oldcol = ::SetTextColor(dc, RGB(255, 255, 255));
				rc2.right = rc.right;
			}

			// Draw the text, the other stuff here was moved upwards due to stealthy style being added
			::ExtTextOut(dc, rc2.left, top, ETO_CLIPPED, rc2, ii->getText(COLUMN_STATUS).c_str(), ii->getText(COLUMN_STATUS).length(), NULL);

			SelectObject(dc, oldFont);
			::SetTextColor(dc, oldcol);

			// New way:
			BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			//bah crap, if we return CDRF_SKIPDEFAULT windows won't paint the icons
			//so we have to do it
			if(cd->iSubItem == 0){
				LVITEM lvItem;
				lvItem.iItem = cd->nmcd.dwItemSpec;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_IMAGE | LVIF_STATE;
				lvItem.stateMask = LVIS_SELECTED;
				ctrlTransfers.GetItem(&lvItem);

				HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlTransfers.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
				if(imageList) {
					//let's find out where to paint it
					//and draw the background to avoid having 
					//the selection color as background
					CRect iconRect;
					ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
					ImageList_Draw(imageList, lvItem.iImage, cd->nmcd.hdc, iconRect.left, iconRect.top, ILD_TRANSPARENT);
				}
			}
			return CDRF_SKIPDEFAULT;
                } else 
					if( /* [-]PPA BOOLSETTING(GET_USER_COUNTRY) && */
                          (colIndex == COLUMN_LOCATION)) { // !SMT!-IP
			ItemInfo* ii = (ItemInfo*)cd->nmcd.lItemlParam;
			ctrlTransfers.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			COLORREF color;
			if(ctrlTransfers.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlTransfers.m_hWnd == ::GetFocus()) {
					color = GetSysColor(COLOR_HIGHLIGHT);
					SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				} else {
					color = GetBkColor(cd->nmcd.hdc);
					SetBkColor(cd->nmcd.hdc, color);
				}				
			} else {
				color = ( ( BOOLSETTING(USE_CUSTOM_LIST_BACKGROUND) && 
					cd->nmcd.dwItemSpec % 2 != 0) ? HLS_TRANSFORM(WinUtil::bgColor, -9, 0) : WinUtil::bgColor);
				SetBkColor(cd->nmcd.hdc, color);
				SetTextColor(cd->nmcd.hdc, WinUtil::textColor);
			}
			CRect rc2 = rc;
			rc2.left += 2;
			HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
			HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
			Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

			DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
			DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

			TCHAR buf[256];
			ctrlTransfers.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				LONG top = rc2.top + (rc2.Height() - 15)/2;
				if((top - rc2.top) < 2)
					top = rc2.top + 1;

				POINT p = { rc2.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagImage, p, LVSIL_SMALL);
				top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
                } else if((colIndex != COLUMN_USER) && (colIndex != COLUMN_HUB) && (colIndex != COLUMN_STATUS) && (colIndex != COLUMN_LOCATION) && // !SMT!-IP
			(ii->status != ItemInfo::STATUS_RUNNING)) {
			cd->clrText = OperaColors::blendColors(WinUtil::bgColor, WinUtil::textColor, 0.4);
			return CDRF_NEWFONT;
		}
		// Fall through
	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT TransferView::onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if (item->iItem != -1) {		
		CRect rect;
		ctrlTransfers.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ItemInfo* i = ctrlTransfers.getItemData(item->iItem);
		if(/*!i->multiSource || */i->subItems.size() <= 1) {
			switch(SETTING(TRANSFERLIST_DBLCLICK)) {
				case 0:
                                        i->pm();
					break;
				case 1:
                                        i->getList();
					break;
				case 2:
                                        i->matchQueue();
					break;
				case 4:
                                        i->addFav();
                                        break;
                                case 5: // !SMT!-UI
                                        i->columns[COLUMN_STATUS] = CTSTRING(CONNECTING_FORCED);
                                        ctrlTransfers.updateItem(i);
                                        ClientManager::getInstance()->connect(i->user);
					break;
			}
		}
	}
	return 0;
}

int TransferView::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
	if(a->status == b->status) {
		if(a->download != b->download) {
			return a->download ? -1 : 1;
		}
	} else {
		return (a->status == ItemInfo::STATUS_RUNNING) ? -1 : 1;
	}

	switch(col) {
		case COLUMN_USER:
			{
				if(a->subItems.size() == b->subItems.size())
					return lstrcmpi(a->columns[COLUMN_USER].c_str(), b->columns[COLUMN_USER].c_str());
				return compare(a->subItems.size(), b->subItems.size());						
			}
		case COLUMN_STATUS: return 0;
		case COLUMN_TIMELEFT: return compare(a->timeLeft, b->timeLeft);
		case COLUMN_SPEED: return compare(a->speed, b->speed);
		case COLUMN_SIZE: return compare(a->size, b->size); 
//[-]PPA		case COLUMN_RATIO: return compare(a->getRatio(), b->getRatio());
		case COLUMN_SHARE: 
			return compare(a->getUser()->getBytesShared(),b->getUser()->getBytesShared());
		case COLUMN_SLOTS: 
			return compare(atoi(Text::fromT(a->columns[col]).c_str()), atoi(Text::fromT(b->columns[col]).c_str()));
		default: 
			return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
	}
}
		
LRESULT TransferView::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;
	tasks.get(t);
	if(t.size() > 2) {
		ctrlTransfers.SetRedraw(FALSE);
	}

	for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {	
		if(i->first == ADD_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
			ItemInfo* ii = new ItemInfo(ui->user, ui->download);
			if(ii->download) {
				ii->streamingDownload = QueueManager::getInstance()->isStreamingDownload(Text::fromT(ui->target));
			}
			ii->update(*ui);
			if(ii->download) {
				ctrlTransfers.insertGroupedItem(ii, false, ui->fileList);
				ii->main->multiSource = ii->multiSource;
			} else {
				ctrlTransfers.insertItem(ii, IMAGE_UPLOAD);
			}
		} else if(i->first == REMOVE_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
					if(ui->download) {
				bool found = false;
				for(ItemInfo::Iter k = ctrlTransfers.mainItems.begin(); k != ctrlTransfers.mainItems.end(); ++k) {
					for(ItemInfo::Iter j = (*k)->subItems.begin(); j != (*k)->subItems.end(); j++) {
						ItemInfo* ii = *j;
						if(*ui == *ii) {
						ctrlTransfers.removeGroupedItem(ii);
							found = true;
							break;
						}
					}
					if(found) break;
				}
					} else {
				int ic = ctrlTransfers.GetItemCount();
				for(int j = 0; j < ic; ++j) {
					const ItemInfo* ii = ctrlTransfers.getItemData(j);
					if(*ui == *ii) {
						ctrlTransfers.DeleteItem(j);
						delete ii;
					break;
				}
			}
			}
		} else if(i->first == UPDATE_ITEM) {
			auto_ptr<UpdateInfo> ui(reinterpret_cast<UpdateInfo*>(i->second));
					if(ui->download) {
				bool found = false;
				for(ItemInfo::Iter k = ctrlTransfers.mainItems.begin(); k != ctrlTransfers.mainItems.end(); ++k) {
					for(ItemInfo::Iter j = (*k)->subItems.begin(); j != (*k)->subItems.end(); j++) {
						ItemInfo* ii = *j;
						if(*ui == *ii) {
						ii->update(*ui);
						if(ii->main) {
							ItemInfo* main = ii->main;
								if(ui->updateMask && UpdateInfo::MASK_FILE) {
									if(main->Target != ii->Target) {
										ctrlTransfers.removeGroupedItem(ii, false);
										ctrlTransfers.insertGroupedItem(ii, false);
										main = ii->main;
									}
								}

							main->multiSource = ii->multiSource;
							bool defString = false;
								
							switch(ii->status) {
								case ItemInfo::DOWNLOAD_STARTING:
									ii->status = ItemInfo::STATUS_RUNNING;
									if(main->status != ItemInfo::STATUS_RUNNING) {
										main->fileBegin = GET_TICK();

										Sounds::PlaySound(SettingsManager::BEGINFILE);

										if(BOOLSETTING(POPUP_DOWNLOAD_START) 
											 && MainFrame::getMainFrame() //[+]PPA fix AV
											 && ii->user
											 ) {
                                                                                           WinUtil::ShowBalloonTip((
												TSTRING(FILE) + _T(": ")+ Util::getFileName(ii->Target) + _T("\n")+
												TSTRING(USER) + _T(": ") + Text::toT(ii->user->getFirstNick())).c_str(), CTSTRING(DOWNLOAD_STARTING));
										}
										main->start = 0;
										main->actual = 0;
										main->pos = 0;
										main->status = ItemInfo::DOWNLOAD_STARTING;
									}
									break;
								case ItemInfo::DOWNLOAD_FINISHED:
									ii->status = ItemInfo::STATUS_WAITING;
									main->status = ItemInfo::STATUS_WAITING;
									main->columns[COLUMN_STATUS] = TSTRING(DOWNLOAD_FINISHED_IDLE);
									break;
								default:
									defString = true;
							}
							 if(mainItemTick(main, ii->status != ItemInfo::STATUS_RUNNING) && 
								 defString && 
								 ui->transferFailed)
								main->columns[COLUMN_STATUS] = ii->columns[COLUMN_STATUS];
					
							if(!main->collapsed)
								ctrlTransfers.updateItem(ii);
							ctrlTransfers.updateItem(main);
						}				
							found = true;
					break;
				}
			}
					if(found) break;
				}
			} else {
				int ic = ctrlTransfers.GetItemCount();
				for(int j = 0; j < ic; ++j) {
					ItemInfo* ii = ctrlTransfers.getItemData(j);
					if(ii->download == ui->download && ii->user == ui->user) {
						ii->update(*ui);
						ctrlTransfers.updateItem(j);
						break;
					}
				}
			}
			}
	}

	if(!t.empty()) {
		ctrlTransfers.resort();
		if(t.size() > 2) {
			ctrlTransfers.SetRedraw(TRUE);
		}
	}
	
	return 0;
}

LRESULT TransferView::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  for (Selection i(this); i.hasNext(); ) {
    ItemInfo *ii = i.next();
    TTHValue tth;
    if(QueueManager::getInstance()->getTTH(Text::fromT(ii->Target), tth)) {
      WinUtil::searchHash(tth);
    }
  }
  return 0;
}
	
TransferView::ItemInfo::ItemInfo(const UserPtr& u, bool aDownload) : UserInfoBase(u), download(aDownload), streamingDownload(false), transferFailed(false),
	status(STATUS_WAITING), pos(0), size(0), start(0), actual(0), speed(0), timeLeft(0),
        Target(Util::emptyStringT), flagImage(0), collapsed(true), main(NULL)
{ 
	columns[COLUMN_USER] = Text::toT(u->getFirstNick());
	columns[COLUMN_HUB] = WinUtil::getHubNames(u).first;
        // !SMT!-UI
    columns[COLUMN_SHARE] = Util::formatBytesW(u->getBytesShared());
    columns[COLUMN_SLOTS] = Util::toStringW(u->getCountSlots());
}

void TransferView::ItemInfo::update(const UpdateInfo& ui) {
	if(ui.updateMask & UpdateInfo::MASK_STATUS) {
		status = ui.status;
	}
	if(ui.updateMask & UpdateInfo::MASK_STATUS_STRING) {
		// No slots etc from transfermanager better than disconnected from connectionmanager
		if(!transferFailed || status == DOWNLOAD_STARTING)
			columns[COLUMN_STATUS] = ui.statusString;
		transferFailed = ui.transferFailed;
	}
	if(ui.updateMask & UpdateInfo::MASK_SIZE) {
		size = ui.size;
		columns[COLUMN_SIZE] = Util::formatBytesW(size);
	}
	if(ui.updateMask & UpdateInfo::MASK_START) {
		start = ui.start;
	}
	if(ui.updateMask & UpdateInfo::MASK_POS) {
		pos = start + ui.pos;
	}
	if(ui.updateMask & UpdateInfo::MASK_ACTUAL) {
		actual = start + ui.actual;
//[-]PPA		columns[COLUMN_RATIO] = Util::toStringW(getRatio());
//[?]		columns[COLUMN_SHARE] = Text::toT(Util::formatBytes(ui.user->getBytesShared())); 
//[?]		columns[COLUMN_SLOTS] = Text::toT(Util::toString(ui.user->getLastCountSlots())); 
	}
	if(ui.updateMask & UpdateInfo::MASK_SPEED) {
		speed = ui.speed;
		if (status == STATUS_RUNNING) {
			columns[COLUMN_SPEED] =Util::formatBytesW(speed) + _T("/s");
		} else {
			columns[COLUMN_SPEED] = Util::emptyStringT;
		}
	}
	if(ui.updateMask & UpdateInfo::MASK_FILE) {
		if(ui.download) Target = ui.target;
		columns[COLUMN_FILE] = ui.file;
		if (streamingDownload)
			columns[COLUMN_FILE] += _T(" [video]");
		columns[COLUMN_PATH] = ui.path;
	}
	if(ui.updateMask & UpdateInfo::MASK_TIMELEFT) {
		timeLeft = ui.timeLeft;
		if (status == STATUS_RUNNING) {
			columns[COLUMN_TIMELEFT] = Util::formatSeconds(timeLeft);
		} else {
			columns[COLUMN_TIMELEFT] = Util::emptyStringT;
		}
	}
	if(ui.updateMask & UpdateInfo::MASK_IP) {
                flagImage = ui.flagImage;
		columns[COLUMN_IP] = Text::toT(ui.ip);
                columns[COLUMN_LOCATION] = ui.location; // !SMT!-IP
#ifdef PPA_INCLUDE_DNS
         columns[COLUMN_DNS] = ui.dns; // !SMT!-IP
#endif
		if(main && main->subItems.size() == 1) {
			main->flagImage = flagImage;
			main->columns[COLUMN_IP] = columns[COLUMN_IP];
#ifdef PPA_INCLUDE_DNS
            main->columns[COLUMN_DNS] = ui.dns; // !SMT!-IP
#endif
            main->columns[COLUMN_LOCATION] = ui.location; // !SMT!-IP
		}
	}
	if(ui.updateMask & UpdateInfo::MASK_SEGMENT) {
		multiSource = ui.multiSource;
	}
}

void TransferView::on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());

	if(ui->download) {
		string aTarget; int64_t aSize; int aFlags; bool segmented;
		if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags, ui->fileList, segmented)) {
			ui->setMultiSource(segmented);
			ui->setFile(Text::toT(aTarget));
			ui->setSize(aSize);
		}
	}

	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(TSTRING(CONNECTING));

	speak(ADD_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	string aTarget;	int64_t aSize; int aFlags = 0; bool segmented;

	if(QueueManager::getInstance()->getQueueInfo(aCqi->getUser(), aTarget, aSize, aFlags, ui->fileList, segmented)) {
		ui->setMultiSource(segmented);
		ui->setFile(Text::toT(aTarget));
		ui->setSize(aSize);
	}

	ui->setStatusString((aFlags & QueueItem::FLAG_TESTSUR) ? TSTRING(CHECKING_CLIENT) : TSTRING(CONNECTING));
	ui->setStatus(ItemInfo::STATUS_WAITING);

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) {
	if(aCqi) //[+]PPA
	speak(REMOVE_ITEM, new UpdateInfo(aCqi->getUser(), aCqi->getDownload()));
}

void TransferView::on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aCqi->getUser(), aCqi->getDownload());
	if(aCqi->getUser()->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(TSTRING(SOURCE_TOO_OLD));
        } 
#ifdef PPA_INCLUDE_IPFILTER
	else if(aCqi->getUser()->isSet(User::PG_BLOCK)) {
                ui->setStatusString(TSTRING(BLOCKED_BY_IPFILTER));
	} 
#endif
#ifdef PPA_INCLUDE_PG
	else if(aCqi->getUser()->isSet(User::PG_BLOCK)) {
                ui->setStatusString(TSTRING(PG_BLOCKED));
	} 
#endif
	else {
		ui->setStatusString(Text::toT(aReason));
	}
	ui->setStatus(ItemInfo::STATUS_WAITING);
	speak(UPDATE_ITEM, ui);
}

// !SMT!-IP
void TransferView::UpdateInfo::setIP(const string& aIP)
{
    ip = aIP;
#ifdef PPA_INCLUDE_DNS
    dns = Text::toT(Socket::nslookup(aIP));
#endif
    string country = Util::getIpCountry(aIP);
    if (!country.empty())
        flagImage = WinUtil::getFlagImage(country);
    location = Text::toT(country);
    updateMask |= MASK_IP;
}


void TransferView::on(DownloadManagerListener::Starting, Download* aDownload) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getUser(), true);
	bool chunkInfo = aDownload->isSet(Download::FLAG_MULTI_CHUNK) && !aDownload->isSet(Download::FLAG_TREE_DOWNLOAD);

	ui->setStatus(ItemInfo::DOWNLOAD_STARTING);
	ui->setPos(chunkInfo ? 0 : aDownload->getTotal());
	ui->setActual(chunkInfo ? 0 : aDownload->getActual());
	ui->setStart(chunkInfo ? 0 : aDownload->getPos());
	ui->setSize(chunkInfo ? aDownload->getChunkSize() : aDownload->getSize());
	ui->setFile(Text::toT(aDownload->getTarget()));
	ui->setStatusString(TSTRING(DOWNLOAD_STARTING));
	ui->setMultiSource(aDownload->isSet(Download::FLAG_MULTI_CHUNK));
        ui->setIP(aDownload->getUserConnection().getRemoteIp()); // !SMT!-IP
	if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		ui->file = _T("TTH: ") + ui->file;
		ui->setStatus(ItemInfo::TREE_DOWNLOAD);
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Tick, const Download::List& dl) {
	AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);

	for(Download::List::const_iterator j = dl.begin(); j != dl.end(); ++j) {
		Download* d = *j;

		UpdateInfo* ui = new UpdateInfo(d->getUser(), true);
		ui->setActual(d->getActual());
		ui->setPos(d->getTotal());
		ui->setTimeLeft(d->getSecondsLeft());
		ui->setSpeed(d->getRunningAverage());

                if (!ui->ip.empty()
#ifdef PPA_INCLUDE_DNS
					  && ui->dns.empty()
#endif
					 ) // !SMT!-IP (delayed updates)
                    ui->setIP(ui->ip);

		if(d->isSet(Download::FLAG_MULTI_CHUNK) && !d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
			ui->setSize(d->isSet(Download::FLAG_MULTI_CHUNK) ? d->getChunkSize() : d->getSize());
			ui->timeLeft = (ui->speed > 0) ? ((ui->size - d->getTotal()) / ui->speed) : 0;

			double progress = (double)(d->getTotal())*100.0/(double)ui->size;
			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(d->getTotal()).c_str(), 
				progress, Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());
			if(progress > 100) {
				// workaround to fix > 100% percentage
				d->getUserConnection().disconnect();
				continue;
			}
		} else {
			_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(d->getPos()).c_str(), 
				(double)d->getPos()*100.0/(double)d->getSize(), Util::formatSeconds((GET_TICK() - d->getStart())/1000).c_str());
		}

		tstring statusString;

		if(d->isSet(Download::FLAG_PARTIAL)) {
			statusString += _T("[P]");
		}
#ifdef PPA_INCLUDE_SSL
		if(d->getUserConnection().isSecure()) {
			if(d->getUserConnection().isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
#endif
		if(d->isSet(Download::FLAG_TTH_CHECK)) {
			statusString += _T("[T]");
		}
		if(d->isSet(Download::FLAG_ZDOWNLOAD)) {
			statusString += _T("[Z]");
		}
		if(d->isSet(Download::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
		if(d->isSet(Download::FLAG_ROLLBACK)) {
			statusString += _T("[R]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}
		statusString += buf;
		ui->setStatusString(statusString);
		if((d->getRunningAverage() == 0) && ((GET_TICK() - d->getStart()) > 1000)) {
			d->getUserConnection().disconnect();
		}
			
		tasks.add(UPDATE_ITEM, ui);
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) {
	UpdateInfo* ui = new UpdateInfo(aDownload->getUser(), true, true);
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(0);
	ui->setStatusString(Text::toT(aReason));
	ui->setSize(aDownload->getSize());
	ui->setFile(Text::toT(aDownload->getTarget()));
        ui->setIP(aDownload->getUserConnection().getRemoteIp()); // !SMT!-IP
	if(BOOLSETTING(POPUP_DOWNLOAD_FAILED)) {
		WinUtil::ShowBalloonTip((
			TSTRING(FILE)+_T(": ") + ui->file + _T("\n")+
			TSTRING(USER)+_T(": ") + Text::toT(ui->user->getFirstNick()) + _T("\n")+
			TSTRING(REASON)+_T(": ") + Text::toT(aReason)).c_str(), CTSTRING(DOWNLOAD_FAILED), NIIF_WARNING);
	}
	if(aDownload->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		ui->file = _T("TTH: ") + ui->file;
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(DownloadManagerListener::Status, const UserPtr& aUser, const string& aMessage) {
	{
		//Lock l(cs);
		for(ItemInfo::List::iterator i = transferItems.begin(); i != transferItems.end(); ++i) {
			ItemInfo* ii = *i;
			if(ii->download && (ii->user == aUser)) {
				if(ii->status == ItemInfo::STATUS_WAITING) return;
				break;
			}
		}
	}
	UpdateInfo* ui = new UpdateInfo(aUser, true, aMessage != STRING(WAITING_TO_RETRY));
	ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(Text::toT(aMessage));
	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Starting, Upload* aUpload) {
	UpdateInfo* ui = new UpdateInfo(aUpload->getUser(), false);

	ui->setStatus(ItemInfo::STATUS_RUNNING);
	ui->setPos(aUpload->getTotal());
	ui->setActual(aUpload->getActual());
	ui->setStart(aUpload->getPos());
	ui->setSize(aUpload->isSet(Upload::FLAG_TTH_LEAVES) ? aUpload->getSize() : aUpload->getFileSize());
	ui->setFile(Text::toT(aUpload->getSourceFile()));

	if(!aUpload->isSet(Upload::FLAG_RESUMED)) {
		ui->setStatusString(TSTRING(UPLOAD_STARTING));
	}

        ui->setIP(aUpload->getUserConnection().getRemoteIp()); // !SMT!-IP
	if(aUpload->isSet(Upload::FLAG_TTH_LEAVES)) {
		ui->file = _T("TTH: ") + ui->file;
	}

	speak(UPDATE_ITEM, ui);
}

void TransferView::on(UploadManagerListener::Tick, const Upload::List& ul) {
	AutoArray<TCHAR> buf(TSTRING(UPLOADED_BYTES).size() + 64);

	for(Upload::List::const_iterator j = ul.begin(); j != ul.end(); ++j) {
		Upload* u = *j;

		if (u->getTotal() == 0) continue;

		UpdateInfo* ui = new UpdateInfo(u->getUser(), false);
		ui->setActual(u->getActual());
		ui->setPos(u->getTotal());
		ui->setTimeLeft(u->getSecondsLeft(true)); // we are interested when whole file is finished and not only one chunk
		ui->setSpeed(u->getRunningAverage());

		_stprintf(buf, CTSTRING(UPLOADED_BYTES), Util::formatBytesW(u->getPos()).c_str(), 
			(double)u->getPos()*100.0/(double)(u->isSet(Upload::FLAG_TTH_LEAVES) ? u->getSize() : u->getFileSize()), Util::formatSeconds((GET_TICK() - u->getStart())/1000).c_str());

		tstring statusString;

		if(u->isSet(Upload::FLAG_PARTIAL_SHARE)) {
			statusString += _T("[P]");
		}
		if(u->isSet(Upload::FLAG_CHUNKED)) {
			statusString += _T("[C]");
		}
#ifdef PPA_INCLUDE_SSL
		if(u->getUserConnection().isSecure()) {
			if(u->getUserConnection().isTrusted()) {
				statusString += _T("[S]");
			} else {
				statusString += _T("[U]");
			}
		}
#endif
		if(u->isSet(Upload::FLAG_ZUPLOAD)) {
			statusString += _T("[Z]");
		}
		if(!statusString.empty()) {
			statusString += _T(" ");
		}			
		statusString += buf;

		ui->setStatusString(statusString);
					
		tasks.add(UPDATE_ITEM, ui);
		if((u->getRunningAverage() == 0) && ((GET_TICK() - u->getStart()) > 1000)) {
			u->getUserConnection().disconnect(true);
		}
	}

	PostMessage(WM_SPEAKER);
}

void TransferView::onTransferComplete(Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree) {
	UpdateInfo* ui = new UpdateInfo(aTransfer->getUser(), !isUpload);
	if(!isUpload) {
		ui->setStatus(isTree ? ItemInfo::STATUS_WAITING : ItemInfo::DOWNLOAD_FINISHED);
		if(BOOLSETTING(POPUP_DOWNLOAD_FINISHED) && !isTree) {
			WinUtil::ShowBalloonTip((
				TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
				TSTRING(USER) + _T(": ") + Text::toT(aTransfer->getUser()->getFirstNick())).c_str(), CTSTRING(DOWNLOAD_FINISHED_IDLE));
		}
	} else {
		ui->setStatus(ItemInfo::STATUS_WAITING);
		if(BOOLSETTING(POPUP_UPLOAD_FINISHED) && !isTree) {
			WinUtil::ShowBalloonTip((
				TSTRING(FILE) + _T(": ") + Text::toT(aFileName) + _T("\n")+
				TSTRING(USER) + _T(": ") + Text::toT(aTransfer->getUser()->getFirstNick())).c_str(), CTSTRING(UPLOAD_FINISHED_IDLE));
		}
	}
	
	ui->setPos(0);
	ui->setStatusString(isUpload ? TSTRING(UPLOAD_FINISHED_IDLE) : TSTRING(DOWNLOAD_FINISHED_IDLE));

	speak(UPDATE_ITEM, ui);
}

void TransferView::ItemInfo::disconnect() {
	ConnectionManager::getInstance()->disconnect(user, download);
}

void TransferView::ItemInfo::setPriorityPause() {
  QueueManager* const qm = QueueManager::getInstance();
  qm->setAutoPriority(Text::fromT(Target), false);
  qm->setPriority(Text::fromT(Target), QueueItem::PAUSED);
}
		
LRESULT TransferView::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);

		QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();

		string tmp = Text::fromT(ii->Target);
		QueueItem::StringIter qi = queue.find(&tmp);

		string aTempTarget;
		if(qi != queue.end())
			aTempTarget = qi->second->getTempTarget();

		QueueManager::getInstance()->unlockQueue();

		WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, aTempTarget);
	}

	return 0;
}

void TransferView::CollapseAll() {
	for(int q = ctrlTransfers.GetItemCount()-1; q != -1; --q) {
		ItemInfo* m = (ItemInfo*)ctrlTransfers.getItemData(q);
		if(m->download && m->main) {
			ctrlTransfers.deleteItem(m); 
		}
		if(m->download && !m->main) {
			m->collapsed = true;
			ctrlTransfers.SetItemState(ctrlTransfers.findItem(m), INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
		 }
	}
}

void TransferView::ExpandAll() {
	for(vector<ItemInfo*>::const_iterator i = ctrlTransfers.mainItems.begin(); i != ctrlTransfers.mainItems.end(); ++i) {
		if((*i)->collapsed) {
			ctrlTransfers.Expand(*i, ctrlTransfers.findItem(*i));
		}
	}
}

LRESULT TransferView::onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
		for(ItemInfo::Iter j = ii->subItems.begin(); j != ii->subItems.end(); ++j) {
			int h = ctrlTransfers.findItem(*j);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(CONNECTING_FORCED));
			ClientManager::getInstance()->connect((*j)->user);
		}
	}
	return 0;
}

LRESULT TransferView::onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = ctrlTransfers.getItemData(i);
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CTSTRING(DISCONNECTED));
		for(ItemInfo::Iter j = ii->subItems.begin(); j != ii->subItems.end(); ++j) {
			int h = ctrlTransfers.findItem(*j);
			if(h != -1)
				ctrlTransfers.SetItemText(h, COLUMN_STATUS, CTSTRING(DISCONNECTED));
			(*j)->disconnect();
		}
	}
	return 0;
}

#ifdef PPA_INCLUDE_DROP_SLOW
LRESULT TransferView::onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while((i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ItemInfo *ii = ctrlTransfers.getItemData(i);
		QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();
		string tmp = Text::fromT(ii->Target);
		QueueItem::StringIter qi = queue.find(&tmp);
		if(qi != queue.end()) {
			if(qi->second->isSet(QueueItem::FLAG_AUTODROP)) {
				qi->second->unsetFlag(QueueItem::FLAG_AUTODROP);
			} else {
				qi->second->setFlag(QueueItem::FLAG_AUTODROP);
			}
		}
		QueueManager::getInstance()->unlockQueue();
	}
	return 0;
}
#endif

void TransferView::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlTransfers.GetBkColor() != WinUtil::bgColor) {
		ctrlTransfers.SetBkColor(WinUtil::bgColor);
		ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
		ctrlTransfers.setFlickerFree(WinUtil::bgBrush);
		refresh = true;
	}
	if(ctrlTransfers.GetTextColor() != WinUtil::textColor) {
		ctrlTransfers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

bool TransferView::mainItemTick(ItemInfo* main, bool smallUpdate) {
	size_t totalSpeed = 0;	double ratio = 0; uint8_t segs = 0;
	ItemInfo* l = NULL;

	for(ItemInfo::Iter k = main->subItems.begin(); k != main->subItems.end(); ++k) {
		l = *k;
		if(l->status == ItemInfo::STATUS_RUNNING/* && main->Target == l->Target*/) {
			segs++;
			if(main->multiSource) {
				totalSpeed += (size_t)l->speed;
				ratio += l->getRatio();
			} else
				break;
		}
	}

	if(!main->multiSource) {
 		main->status = l->status;
		main->size = l->size;
		main->pos = l->pos;
		main->actual = l->actual;
		main->speed = l->speed;
		main->timeLeft = l->timeLeft;
		main->columns[COLUMN_HUB] = l->columns[COLUMN_HUB];
		main->columns[COLUMN_STATUS] = l->columns[COLUMN_STATUS];
		main->columns[COLUMN_SIZE] = Util::formatBytesW(l->size);
		main->columns[COLUMN_SPEED] = Util::formatBytesW(l->speed) + _T("/s");
		main->columns[COLUMN_TIMELEFT] = Util::formatSeconds(l->timeLeft);
//[-]PPA		main->columns[COLUMN_RATIO] = Util::toStringW(l->getRatio());
		main->columns[COLUMN_SLOTS] = Text::toT(Util::toString(l->getUser()->getCountSlots()));
		main->columns[COLUMN_SHARE] = Text::toT(Util::formatBytes(l->getUser()->getBytesShared()));

		return false;
	} else if(segs == 0) {
		main->pos = 0;
		main->actual = 0;
		main->status = ItemInfo::STATUS_WAITING;
		main->fileBegin = 0;
		main->timeLeft = 0;
		main->speed = 0;
		main->columns[COLUMN_TIMELEFT] = Util::emptyStringT;
		main->columns[COLUMN_SPEED] = Util::emptyStringT;
        main->columns[COLUMN_SLOTS] = Util::emptyStringT;
        main->columns[COLUMN_SHARE] = Util::emptyStringT;

//[-]PPA		main->columns[COLUMN_RATIO] = Util::emptyStringT;
		if(main->multiSource && main->subItems.size() > 1)
			main->columns[COLUMN_HUB] = _T("0 ") + TSTRING(NUMBER_OF_SEGMENTS);
		
		return true;
	} else {
		if(smallUpdate) return false;

		ratio = ratio / segs;

		int64_t total = 0;
		int64_t fileSize = -1;

		string tmp = Text::fromT(main->Target);
		QueueItem::StringMap& queue = QueueManager::getInstance()->lockQueue();
		QueueItem::StringIter qi = queue.find(&tmp);
		if(qi != queue.end()) {
                	QueueItem* pQueueItem = qi->second;
			total = pQueueItem->getDownloadedBytes();
			fileSize = pQueueItem->getSize();
		}
		QueueManager::getInstance()->unlockQueue();

		if(fileSize == -1) return true;

		if(main->status == ItemInfo::DOWNLOAD_STARTING) {
			main->status = ItemInfo::STATUS_RUNNING;
			main->columns[COLUMN_STATUS] = TSTRING(DOWNLOAD_STARTING);
		} else {
			AutoArray<TCHAR> buf(TSTRING(DOWNLOADED_BYTES).size() + 64);		
				_stprintf(buf, CTSTRING(DOWNLOADED_BYTES), Util::formatBytesW(total).c_str(), 
				(double)total*100.0/(double)fileSize, Util::formatSeconds((GET_TICK() - main->fileBegin)/1000).c_str());

			tstring statusString;
			// TODO statusString += _T("[T]");

			// hack to display whether file is compressed
			if(ratio < 1.0000) {
				statusString += _T("[Z] ");
			}
			statusString += buf;
			main->columns[COLUMN_STATUS] = statusString;
			
			main->actual = (int64_t)(total * ratio);
			main->pos = total;
			main->speed = totalSpeed;
			main->timeLeft = (totalSpeed > 0) ? ((fileSize - main->pos) / totalSpeed) : 0;
		}
		main->size = fileSize;
		
		if(main->subItems.size() > 1) {
			TCHAR buf[64];
			snwprintf(buf, sizeof(buf), _T("%d %s"), segs, CTSTRING(NUMBER_OF_SEGMENTS));

			main->columns[COLUMN_HUB] = buf;
		}
		main->columns[COLUMN_SIZE] = Util::formatBytesW(fileSize);
		main->columns[COLUMN_SPEED] = Util::formatBytesW(main->speed) + _T("/s");
		main->columns[COLUMN_TIMELEFT] = Util::formatSeconds(main->timeLeft);
        if(main->getUser())
		 {
		if(const int l_cnt_slots = main->getUser()->getCountSlots())
		   main->columns[COLUMN_SLOTS] = Text::toT(Util::toString(l_cnt_slots));
		const int64_t l_share = main->getUser()->getBytesShared();
		if(l_share >= 0)
          main->columns[COLUMN_SHARE] = Text::toT(Util::formatBytes(l_share));
          }
//[-]PPA		main->columns[COLUMN_RATIO] = Util::toStringW(ratio);
		return false;
	}
}

// !SMT!-S
LRESULT TransferView::onSetUserLimit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   FavoriteUser::UPLOAD_LIMIT lim;
   switch (wID) {
      case IDC_SPEED_NONE:  lim = FavoriteUser::UL_NONE; break;
      case IDC_SPEED_SUPER: lim = FavoriteUser::UL_SU;   break;
      case IDC_SPEED_BAN:   lim = FavoriteUser::UL_BAN;  break;
      case IDC_SPEED_02K:   lim = FavoriteUser::UL_2;    break;
      case IDC_SPEED_05K:   lim = FavoriteUser::UL_5;    break;
      case IDC_SPEED_08K:   lim = FavoriteUser::UL_8;    break;
      case IDC_SPEED_12K:   lim = FavoriteUser::UL_12;   break;
      case IDC_SPEED_16K:   lim = FavoriteUser::UL_16;   break;
      case IDC_SPEED_24K:   lim = FavoriteUser::UL_24;   break;
      case IDC_SPEED_32K:   lim = FavoriteUser::UL_32;   break;
      case IDC_SPEED_64K:   lim = FavoriteUser::UL_64;   break;
      case IDC_SPEED_128K:  lim = FavoriteUser::UL_128;  break;
      case IDC_SPEED_256K:  lim = FavoriteUser::UL_256;  break;
      default: return 0;
   }

   FavoriteManager::getInstance()->addFavoriteUser(selectedUser);
   FavoriteManager::getInstance()->setUploadLimit(selectedUser, lim);

   // close favusers window (it contains incorrect info, too lazy to update)
   UsersFrame::closeWindow();
   return 0;
}

LRESULT TransferView::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
  POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
  if (pt.y < m_headerHeight) { // если мышь нажата на заголовке
    m_mouseClicked = true;
    m_mouseClickPoint = pt;
  }
  bHandled = FALSE;
  return 0;
}

LRESULT TransferView::onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
  if (m_mouseClicked) {
    m_mouseClicked = false;
    if (m_mouseClickPoint.x >= 0 && m_mouseClickPoint.y >= 0) {
      POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
      CRect r;
      GetClientRect(r);
      if (PtInRect(&r, pt)) {
        const int deltaX = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int deltaY = GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        if (abs(m_mouseClickPoint.x - pt.x) < deltaX && abs(m_mouseClickPoint.y - pt.y) < deltaY) {
          PostMessage(WM_COMMAND, MAKEWPARAM(IDC_TRANSFER_VIEW_TOGGLE, BN_CLICKED), (LPARAM) toggleButton.m_hWnd);
        }
      }
    }
  }
  bHandled = FALSE;
  return 0;
}

/**
 * @file
 * $Id: TransferView.cpp,v 1.21 2008/03/31 15:26:47 alexey Exp $
 */
