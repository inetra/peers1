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

#if !defined(FINISHED_FRAME_BASE_H)
#define FINISHED_FRAME_BASE_H

#include "TypedListViewCtrl.h"
#include "ShellContextMenu.h"
#include "TextFrame.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"
#include "../client/FinishedManager.h"

#include "../peers/FrameHeader.h"

template<class T, int title, int id, int icon>
class FinishedFrameBase : 
  public MDITabChildWindowImpl<T, icon>, 
  public StaticFrame<T, title, id>,
  protected FinishedManagerListener, 
  private SettingsManagerListener
{
private:
  FrameHeader m_header;
public:
	typedef MDITabChildWindowImpl<T, icon> baseClass;

	FinishedFrameBase() : totalBytes(0), totalTime(0), closed(false) { }
	virtual ~FinishedFrameBase() { }

        virtual bool isLargeIcon() const { return true; }

	BEGIN_MSG_MAP(T)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_TOTAL, onRemove)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE, onOpenFile)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenFolder)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrant)		
		NOTIFY_HANDLER(id, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(id, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(id, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(id, NM_DBLCLK, onDoubleClick)	
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
          m_header.Create(m_hWnd);
          m_header.addWords(WSTRING_I((ResourceManager::Strings) title));
		CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
		ctrlStatus.Attach(m_hWndStatusBar);

		ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_SINGLESEL, WS_EX_CLIENTEDGE, id);
		ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);

		ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlList.SetTextColor(WinUtil::textColor);

		// Create listview columns
		WinUtil::splitTokens(columnIndexes, SettingsManager::getInstance()->get(columnOrder), COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SettingsManager::getInstance()->get(columnWidth), COLUMN_LAST);

		for(uint8_t j=0; j<COLUMN_LAST; j++) {
			int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
			ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
		}

		ctrlList.setColumnOrderArray(COLUMN_LAST, columnIndexes);
		ctrlList.setVisible(SettingsManager::getInstance()->get(columnVisible));
		ctrlList.setSortColumn(COLUMN_DONE);

		UpdateLayout();

		SettingsManager::getInstance()->addListener(this);
		FinishedManager::getInstance()->addListener(this);
		updateList(FinishedManager::getInstance()->lockList(upload));
		FinishedManager::getInstance()->unlockList();

		ctxMenu.CreatePopupMenu();
		ctxMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
		ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FILE, CTSTRING(OPEN));
		ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
		ctxMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
		ctxMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
		ctxMenu.AppendMenu(MF_SEPARATOR);
		ctxMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		ctxMenu.AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
		ctxMenu.SetMenuDefaultItem(IDC_OPEN_FILE);

		bHandled = FALSE;
		return TRUE;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if(!closed) {
			FinishedManager::getInstance()->removeListener(this);
			SettingsManager::getInstance()->removeListener(this);

			closed = true;
			WinUtil::setButtonPressed(id, false);
			PostMessage(WM_CLOSE);
			return 0;
		} else {
			ctrlList.saveHeaderOrder(columnOrder, columnWidth, columnVisible);

			//cleanup to avoid memory leak
			for(int i = 0; i < ctrlList.GetItemCount(); ++i) {
				delete ctrlList.getItemData(i);
			}
			ctrlList.DeleteAllItems();

			bHandled = FALSE;
			return 0;
		}
	}

	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

		if(item->iItem != -1) {
			ItemInfo *ii = ctrlList.getItemData(item->iItem);
			WinUtil::openFile(Text::toT(ii->entry->getTarget()));
		}
		return 0;
	}

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		if(wParam == SPEAK_ADD_LINE) {
			FinishedItem* entry = reinterpret_cast<FinishedItem*>(lParam);
			addEntry(entry);
			if(SettingsManager::getInstance()->get(boldFinished))
				setDirty();
			updateStatus();
		} else if(wParam == SPEAK_REMOVE) {
			updateStatus();
		} else if(wParam == SPEAK_REMOVE_ALL) {
			//cleanup to avoid memory leak
			for(int i = 0; i < ctrlList.GetItemCount(); ++i) {
				delete ctrlList.getItemData(i);
			}
			ctrlList.DeleteAllItems();
			updateStatus();
		}
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		switch(wID)
		{
		case IDC_REMOVE:
			{
				int i = -1;
				while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
					ItemInfo *ii = ctrlList.getItemData(i);
					FinishedManager::getInstance()->remove(ii->entry, upload);
					ctrlList.DeleteItem(i);
					delete ii;
				}
				break;
			}
		case IDC_TOTAL:
			FinishedManager::getInstance()->removeAll(upload);
			break;
		}
		return 0;
	}

	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ItemInfo *ii = ctrlList.getItemData(i);
			if(ii != NULL)
				TextFrame::openWindow(Text::toT(ii->entry->getTarget()));
		}
		return 0;
	}

	LRESULT onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ItemInfo *ii = ctrlList.getItemData(i);
			if(ii != NULL)
				WinUtil::openFile(Text::toT(ii->entry->getTarget()));
		}
		return 0;
	}

	LRESULT onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ItemInfo *ii = ctrlList.getItemData(i);
			if(ii != NULL)
				::ShellExecute(NULL, NULL, Text::toT(Util::getFilePath(ii->entry->getTarget())).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		return 0;
	}

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if (reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) { 
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlList, pt);
			}

			bool bShellMenuShown = false;
			if(BOOLSETTING(SHOW_SHELL_MENU) && (ctrlList.GetSelectedCount() == 1)) {
				string path = ((FinishedItem*)ctrlList.GetItemData(ctrlList.GetSelectedIndex()))->getTarget();
				if(File::getSize(path) != 1) {
					CShellContextMenu shellMenu;
					shellMenu.SetPath(Text::toT(path));

					CMenu* pShellMenu = shellMenu.GetMenu();
					pShellMenu->AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
					pShellMenu->AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
					pShellMenu->AppendMenu(MF_SEPARATOR);
					pShellMenu->AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
					pShellMenu->AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
					pShellMenu->AppendMenu(MF_SEPARATOR);

					UINT idCommand = shellMenu.ShowContextMenu(m_hWnd, pt);
					if(idCommand != 0)
						PostMessage(WM_COMMAND, idCommand);

					bShellMenuShown = true;
				}
			}

			if(!bShellMenuShown)
			ctxMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);			

			return TRUE; 
		}
		bHandled = FALSE;
		return FALSE; 
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = reinterpret_cast<NMLVKEYDOWN*>(pnmh);

		if(kd->wVKey == VK_DELETE) {
			PostMessage(WM_COMMAND, IDC_REMOVE);
		} 
		return 0;
	}

	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ItemInfo *ii = ctrlList.getItemData(i);
			if(ii) {
				UserPtr u = ClientManager::getInstance()->findUser(ii->entry->getCID());
				if(u) {
					QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW);
				} else {
					addStatusLine(TSTRING(USER_OFFLINE));
				}
			} else {
				addStatusLine(TSTRING(USER_OFFLINE));
			}

		}
		return 0;
	}

	LRESULT onGrant(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ItemInfo *ii = ctrlList.getItemData(i);
			if(ii) {
				UserPtr u = ClientManager::getInstance()->findUser(ii->entry->getCID());
				if(u) {
					UploadManager::getInstance()->reserveSlot(u, 600);
				} else {
					addStatusLine(TSTRING(USER_OFFLINE));
				}
			} else {
				addStatusLine(TSTRING(USER_OFFLINE));
			}
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE) {
		RECT rect;
		GetClientRect(&rect);

		// position bars and offset their dimensions
		UpdateBarsPosition(rect, bResizeBars);
                m_header.updateLayout(rect.left, rect.top, rect.right);
                rect.top += m_header.getPreferredHeight();

		if(ctrlStatus.IsWindow()) {
			CRect sr;
			int w[4];
			ctrlStatus.GetClientRect(sr);
			w[3] = sr.right - 16;
			w[2] = max(w[3] - 100, 0);
			w[1] = max(w[2] - 100, 0);
			w[0] = max(w[1] - 100, 0);

			ctrlStatus.SetParts(4, w);
		}

		CRect rc(rect);
		ctrlList.MoveWindow(rc);
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlList.SetFocus();
		return 0;
	}

protected:
	enum {
		SPEAK_ADD_LINE,
		SPEAK_REMOVE,
		SPEAK_REMOVE_ALL
	};

	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_DONE,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SIZE,
		COLUMN_SPEED,
		COLUMN_LAST
	};

	class ItemInfo {
	public:
		ItemInfo(FinishedItem* fi) : entry(fi) {
			columns[COLUMN_FILE]  = Text::toT(Util::getFileName(entry->getTarget()));
			columns[COLUMN_DONE]  = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));
			columns[COLUMN_PATH]  = Text::toT(Util::getFilePath(entry->getTarget()));
			columns[COLUMN_NICK]  = Text::toT(entry->getUser());
			columns[COLUMN_HUB]   = Text::toT(entry->getHub());
			columns[COLUMN_SIZE]  = Util::formatBytesW(entry->getSize());
			columns[COLUMN_SPEED] = Util::formatBytesW(entry->getAvgSpeed()) + _T("/s");
		}
		tstring columns[COLUMN_LAST];

		const tstring& getText(int col) const {
			dcassert(col >= 0 && col < COLUMN_LAST);
			return columns[col];
		}

		const tstring& copy(int col) {
			if(col >= 0 && col < COLUMN_LAST)
				return getText(col);

			return Util::emptyStringT;
		}

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
			switch(col) {
				case COLUMN_SPEED:	return compare(a->entry->getAvgSpeed(), b->entry->getAvgSpeed());
				case COLUMN_SIZE:	return compare(a->entry->getSize(), b->entry->getSize());
				default:			return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
			}
		}

		int imageIndex() const { return WinUtil::getIconIndex(Text::toT(entry->getTarget())); }

		FinishedItem* entry;
	};

	CStatusBarCtrl ctrlStatus;
	CMenu ctxMenu;

	TypedListViewCtrl<ItemInfo, id> ctrlList;

	int64_t totalBytes;
	int64_t totalTime;

	bool closed;

	bool upload;
	SettingsManager::IntSetting boldFinished;
	SettingsManager::StrSetting columnWidth;
	SettingsManager::StrSetting columnOrder;
	SettingsManager::StrSetting columnVisible;
	

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	void addStatusLine(const tstring& aLine) {
		ctrlStatus.SetText(0, (Text::toT(Util::getShortTimeString()) + _T(" ") + aLine).c_str());
	}

	void updateStatus() {
		ctrlStatus.SetText(1, (Util::toStringW(ctrlList.GetItemCount()) + _T(" ") + TSTRING(ITEMS)).c_str());
		ctrlStatus.SetText(2, Util::formatBytesW(totalBytes).c_str());
		ctrlStatus.SetText(3, (Util::formatBytesW((totalTime > 0) ? totalBytes * ((int64_t)1000) / totalTime : 0) + _T("/s")).c_str());
	}

	void updateList(const FinishedItem::List& fl) {
		ctrlList.SetRedraw(FALSE);
		for(FinishedItem::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			addEntry(*i);
		}
		ctrlList.SetRedraw(TRUE);
		ctrlList.Invalidate();
		updateStatus();
	}

	void addEntry(FinishedItem* entry) {
		ItemInfo *ii = new ItemInfo(entry);
		totalBytes += entry->getChunkSize();
		totalTime += entry->getMilliSeconds();

		int image = WinUtil::getIconIndex(Text::toT(entry->getTarget()));
		int loc = ctrlList.insertItem(ii, image);
		ctrlList.EnsureVisible(loc, FALSE);
	}

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
		bool refresh = false;
		if(ctrlList.GetBkColor() != WinUtil::bgColor) {
			ctrlList.SetBkColor(WinUtil::bgColor);
			ctrlList.SetTextBkColor(WinUtil::bgColor);
			refresh = true;
		}
		if(ctrlList.GetTextColor() != WinUtil::textColor) {
			ctrlList.SetTextColor(WinUtil::textColor);
			refresh = true;
		}
		if(refresh == true) {
			RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
	}

};

template <class T, int title, int id, int icon>
int FinishedFrameBase<T, title, id, icon>::columnIndexes[] = { COLUMN_DONE, COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_SIZE, COLUMN_SPEED };

template <class T, int title, int id, int icon>
int FinishedFrameBase<T, title, id, icon>::columnSizes[] = { 100, 110, 290, 125, 80, 80, 80 };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::TIME, ResourceManager::PATH, 
ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::SPEED, ResourceManager::CRC_CHECKED
};

#endif // !defined(FINISHED_FRAME_BASE_H)

/**
* @file
* $Id: FinishedFrameBase.h,v 1.9 2008/03/10 07:42:28 alexey Exp $
*/
