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

#include "QueueFrame.h"
#include "../peers/PrivateFrameFactory.h"

#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../peers/PeersVersion.h"
#include "../peers/ControlAdjuster.h"
#include "BarShader.h"

#define BUTTON_WIDTH   36
#define BUTTON_HEIGHT  36
#define H_GAP           8

#define FILE_LIST_NAME _T("File Lists")

int QueueFrame::columnIndexes[] = { COLUMN_TARGET, COLUMN_STATUS, COLUMN_SEGMENTS, COLUMN_SIZE, COLUMN_PROGRESS, COLUMN_DOWNLOADED, COLUMN_PRIORITY,
COLUMN_USERS, COLUMN_PATH, COLUMN_EXACT_SIZE, COLUMN_ERRORS, COLUMN_ADDED, COLUMN_TTH, COLUMN_TYPE };

int QueueFrame::columnSizes[] = { 200, 300, 70, 75, 100, 120, 75, 200, 200, 75, 200, 100, 125, 75 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::STATUS, ResourceManager::SEGMENTS, ResourceManager::SIZE, 
ResourceManager::DOWNLOADED_PARTS, ResourceManager::DOWNLOADED,
ResourceManager::PRIORITY, ResourceManager::USERS, ResourceManager::PATH, ResourceManager::EXACT_SIZE, ResourceManager::ERRORS,
ResourceManager::ADDED, ResourceManager::TTH_ROOT, ResourceManager::TYPE };

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
        m_header.Create(m_hWnd);
        m_header.addWords(TSTRING(DOWNLOAD_QUEUE_TITLE));
        m_btnPause.Create(IDI_PEERS_DOWNLOAD_PAUSE, m_header, NULL, IDC_DOWNLOAD_START_PAUSE);
        m_btnPause.loadAltIcon(IDI_PEERS_DOWNLOAD_START);
        m_btnPause.setAllowFocus(false);
        m_btnPause.setHintText(TSTRING(DOWNLOAD_PAUSE_HINT));
        m_btnDelete.Create(IDI_PEERS_DOWNLOAD_DELETE, m_header, NULL, IDC_DOWNLOAD_DELETE);
        m_btnDelete.setAllowFocus(false);
        m_btnDelete.setHintText(TSTRING(DOWNLOAD_DELETE_HINT));
        m_prioritySlider.Create(m_header);
        m_priorityLabel.Create(m_header, rcDefault, CTSTRING(DOWNLOAD_PRIORITY_LABEL), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        m_priorityLabel.SetFont(m_priorityLabelFont);
        m_priorityValue.Create(m_header, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        m_priorityValue.SetFont(WinUtil::systemFont);
        m_priorityValue.SetWindowLong(GWL_USERDATA, -1);
	showTree = BOOLSETTING(QUEUEFRAME_SHOW_TREE);
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | LVS_EX_INFOTIP);

	ctrlDirs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, 
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	ctrlDirs.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlQueue.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	
	m_nProportionalPos = 2500;
	SetSplitterPanes(ctrlDirs.m_hWnd, ctrlQueue.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);
	
	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_DOWNLOADED || j == COLUMN_EXACT_SIZE|| j == COLUMN_SEGMENTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlQueue.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_TARGET);
	ctrlQueue.setVisible(SETTING(QUEUEFRAME_VISIBLE));
	
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);

	ctrlDirs.SetBkColor(WinUtil::bgColor);
	ctrlDirs.SetTextColor(WinUtil::textColor);

	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);
	
	singleMenu.CreatePopupMenu();
	multiMenu.CreatePopupMenu();
	browseMenu.CreatePopupMenu();
	removeMenu.CreatePopupMenu();
	removeAllMenu.CreatePopupMenu();
	pmMenu.CreatePopupMenu();
	priorityMenu.CreatePopupMenu();
	dirMenu.CreatePopupMenu();	
	readdMenu.CreatePopupMenu();
	previewMenu.CreatePopupMenu();
	segmentsMenu.CreatePopupMenu();

	copyMenu.CreatePopupMenu();
        copyMenu.InsertSeparatorFirst(TSTRING(COPY)); // !SMT!-UI
	for(int i = 0; i <COLUMN_LAST; ++i)
		copyMenu.AppendMenu(MF_STRING, IDC_COPY + i, CTSTRING_I(columnNames[i]));
        copyMenu.AppendMenu(MF_SEPARATOR); // !SMT!-UI
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK)); // !SMT!-UI
        copyMenu.AppendMenu(MF_STRING, IDC_COPY_WMLINK, CTSTRING(COPY_MLINK_TEMPL)); // !SMT!-UI

	singleMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));	
	singleMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)browseMenu, CTSTRING(GET_FILE_LIST));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)pmMenu, CTSTRING(SEND_PRIVATE_MESSAGE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)readdMenu, CTSTRING(READD_SOURCE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
	singleMenu.AppendMenu(MF_SEPARATOR);
	singleMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	singleMenu.AppendMenu(MF_SEPARATOR);
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)removeMenu, CTSTRING(REMOVE_SOURCE));
	singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)removeAllMenu, CTSTRING(REMOVE_FROM_ALL));
	singleMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
	singleMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
        singleMenu.InsertSeparatorFirst(TSTRING(FILE)); // !SMT!-UI
        singleMenu.SetMenuDefaultItem(IDC_SEARCH_ALTERNATES); // !SMT!-UI

        multiMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES)); // !SMT!-UI
        multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
        multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
        multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
        multiMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
        multiMenu.AppendMenu(MF_SEPARATOR);
        multiMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
        multiMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
        multiMenu.SetMenuDefaultItem(IDC_SEARCH_ALTERNATES); // !SMT!-UI
        multiMenu.InsertSeparatorFirst(TSTRING(FILES)); // !SMT!-UI

        const bool advancedMenu = BOOLSETTING(MENU_ADVANCED);
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
        if (advancedMenu) {
          priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST, CTSTRING(LOWEST));
        }
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW, CTSTRING(LOW));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL, CTSTRING(NORMAL));
        priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH, CTSTRING(HIGH));
        if (advancedMenu) {
          priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST, CTSTRING(HIGHEST));
        }
        priorityMenu.AppendMenu(MF_STRING, IDC_AUTOPRIORITY, CTSTRING(AUTO));

        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTONE, (_T("1 ")+TSTRING(SEGMENT)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTWO, (_T("2 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTHREE, (_T("3 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTFOUR, (_T("4 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTFIVE, (_T("5 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTSIX, (_T("6 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTSEVEN, (_T("7 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTEIGHT, (_T("8 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTNINE, (_T("9 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTEN, (_T("10 ")+TSTRING(SEGMENTS)).c_str());

        // !BUGMASTER!-S
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTFIFTY, (_T("50 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTHUNDRED, (_T("100 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTHUNDRED_FIFTY, (_T("150 ")+TSTRING(SEGMENTS)).c_str());
        segmentsMenu.AppendMenu(MF_STRING, IDC_SEGMENTTWO_HUNDRED, (_T("200 ")+TSTRING(SEGMENTS)).c_str());

 	dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
	dirMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
	dirMenu.AppendMenu(MF_SEPARATOR);
	dirMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

	removeMenu.AppendMenu(MF_STRING, IDC_REMOVE_SOURCE, CTSTRING(ALL));
	removeMenu.AppendMenu(MF_SEPARATOR);

	readdMenu.AppendMenu(MF_STRING, IDC_READD, CTSTRING(ALL));
	readdMenu.AppendMenu(MF_SEPARATOR);

	addQueueList(QueueManager::getInstance()->lockQueue());
	QueueManager::getInstance()->unlockQueue();
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	updateStatus();

	bHandled = FALSE;
	return 1;
}

void QueueFrame::QueueItemInfo::update() {
		int colMask = updateMask;
		updateMask = 0;

		QueueItem* qi = QueueManager::getInstance()->fileQueue.find(getTarget());
		m_columns[COLUMN_SEGMENTS] = Util::toStringW(qi ? qi->getCurrents().size() : 0) + _T("/") + Util::toStringW(qi ? qi->getMaxSegments() : 0);// + _T(" ");

		if(colMask & MASK_TARGET) {
			m_columns[COLUMN_TARGET] = Text::toT(Util::getFileName(getTarget()));
			if (qi && qi->isSet(QueueItem::FLAG_ONLINE_VIDEO)) {
				m_columns[COLUMN_TARGET] += _T(" [video]");
			}
		}
		int online = 0;
		if(colMask & MASK_USERS || colMask & MASK_STATUS) {
			tstring tmp;

			for(QueueItem::SourceIter j = getSources().begin(); j != getSources().end(); ++j) {
				if(tmp.size() > 0)
					tmp += _T(", ");

				if(j->getUser()->isOnline())
					online++;

				tmp += Text::toT(j->getUser()->getFirstNick());
			}
			m_columns[COLUMN_USERS] = tmp.empty() ? TSTRING(NO_USERS) : tmp;
		}
		if(colMask & MASK_STATUS) {
			if(getStatus() == QueueItem::STATUS_WAITING) {

				TCHAR buf[64];
				if(online > 0) {
					if(getSources().size() == 1) {
						m_columns[COLUMN_STATUS] = TSTRING(WAITING_USER_ONLINE);
					} else {
						_stprintf(buf, CTSTRING(WAITING_USERS_ONLINE), online, getSources().size());
						m_columns[COLUMN_STATUS] = buf;
					}
				} else {
					if(getSources().empty()) {
						m_columns[COLUMN_STATUS] = TSTRING(NO_USERS_TO_DOWNLOAD_FROM);
					} else if(getSources().size() == 1) {
						m_columns[COLUMN_STATUS] = TSTRING(USER_OFFLINE);
					} else if(getSources().size() == 2) {
						m_columns[COLUMN_STATUS] = TSTRING(BOTH_USERS_OFFLINE);
					} else if(getSources().size() == 3) {
						m_columns[COLUMN_STATUS] = TSTRING(ALL_3_USERS_OFFLINE);
					} else if(getSources().size() == 4) {
						m_columns[COLUMN_STATUS] = TSTRING(ALL_4_USERS_OFFLINE);
					} else {
						_stprintf(buf, CTSTRING(ALL_USERS_OFFLINE), getSources().size());
						m_columns[COLUMN_STATUS] = buf;
					}
				}
			} else if(getStatus() == QueueItem::STATUS_RUNNING) {
				TCHAR buf[64];
				if(online > 0) {
					if(getSources().size() == 1) {
						m_columns[COLUMN_STATUS] = TSTRING(USER_ONLINE);
					} else {
						_stprintf(buf, CTSTRING(USERS_ONLINE), online, getSources().size());
						m_columns[COLUMN_STATUS] = buf;
					}
				} else {
					m_columns[COLUMN_STATUS] = TSTRING(RUNNING);
				}
			} 
		}
		if(colMask & MASK_SIZE) {
			m_columns[COLUMN_SIZE] = (getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatBytesW(getSize());
			m_columns[COLUMN_EXACT_SIZE] = (getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatExactSize(getSize());
		}
		if(colMask & MASK_DOWNLOADED) {
				if(getSize() > 0)
					m_columns[COLUMN_DOWNLOADED] = Util::formatBytesW(getDownloadedBytes()) + _T(" (") + Util::toStringW((double)getDownloadedBytes()*100.0/(double)getSize()) + _T("%)");
				else
					m_columns[COLUMN_DOWNLOADED].clear();
		}
		if(colMask & MASK_PRIORITY) {
			switch(getPriority()) {
				case QueueItem::PAUSED: m_columns[COLUMN_PRIORITY] = TSTRING(PAUSED); break;
				case QueueItem::LOWEST: m_columns[COLUMN_PRIORITY] = TSTRING(LOWEST); break;
				case QueueItem::LOW: m_columns[COLUMN_PRIORITY] = TSTRING(LOW); break;
				case QueueItem::NORMAL: m_columns[COLUMN_PRIORITY] = TSTRING(NORMAL); break;
				case QueueItem::HIGH: m_columns[COLUMN_PRIORITY] = TSTRING(HIGH); break;
				case QueueItem::HIGHEST: m_columns[COLUMN_PRIORITY] = TSTRING(HIGHEST); break;
				default: dcasserta(0); break;
			}
			if(getAutoPriority()) {
				m_columns[COLUMN_PRIORITY] += _T(" (") + TSTRING(AUTO) + _T(")");
			}
		}

		if(colMask & MASK_PATH) {
			m_columns[COLUMN_PATH] = Text::toT(getPath());
		}

		if(colMask & MASK_ERRORS) {
			tstring tmp;
			for(QueueItem::SourceIter j = getBadSources().begin(); j != getBadSources().end(); ++j) {
				if(!j->isSet(QueueItem::Source::FLAG_REMOVED)) {
				if(tmp.size() > 0)
					tmp += _T(", ");
					tmp += Text::toT(j->getUser()->getFirstNick());
					tmp += _T(" (");
					if(j->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						tmp += TSTRING(FILE_NOT_AVAILABLE);
					} else if(j->isSet(QueueItem::Source::FLAG_PASSIVE)) {
						tmp += TSTRING(PASSIVE_USER);
					} else if(j->isSet(QueueItem::Source::FLAG_ROLLBACK_INCONSISTENCY)) {
						tmp += TSTRING(ROLLBACK_INCONSISTENCY);
					} else if(j->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						tmp += TSTRING(INVALID_TREE);
					} else if(j->isSet(QueueItem::Source::FLAG_SLOW)) {
						tmp += TSTRING(SLOW_USER);
					} else if(j->isSet(QueueItem::Source::FLAG_NO_TTHF)) {
						tmp += TSTRING(SOURCE_TOO_OLD);						
					} else if(j->isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
						tmp += TSTRING(NO_NEEDED_PART);
					}
					tmp += _T(')');
				}
			}
			m_columns[COLUMN_ERRORS] = tmp.empty() ? TSTRING(NO_ERRORS) : tmp;
		}

		if(colMask & MASK_ADDED) {
			m_columns[COLUMN_ADDED] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getAdded()));
		}
		if(colMask & MASK_TTH) {
			m_columns[COLUMN_TTH] = Text::toT(getTTH().toBase32());
		}
		if(colMask & MASK_TYPE) {
			m_columns[COLUMN_TYPE] = Text::toT(Util::getFileExt(getTarget()));
			if(m_columns[COLUMN_TYPE].size() > 0 && m_columns[COLUMN_TYPE][0] == '.')
				m_columns[COLUMN_TYPE].erase(0, 1);
		}
}

void QueueFrame::on(QueueManagerListener::Added, QueueItem* aQI) {
	QueueItemInfo* ii = new QueueItemInfo(*aQI);

	speak(ADD_ITEM,	new QueueItemInfoTask(ii));
}

void QueueFrame::addQueueItem(QueueItemInfo* ii, bool noSort) {
	if(!ii->isSet(QueueItem::FLAG_USER_LIST) && !ii->isSet(QueueItem::FLAG_TESTSUR)) {
		queueSize+=ii->getSize();
	}
	queueItems++;
	dirty = true;
	
	const string& dir = ii->getPath();
	
	bool updateDir = (directories.find(dir) == directories.end());
	directories.insert(make_pair(dir, ii));
	
	if(updateDir) {
		addDirectory(dir, ii->isSet(QueueItem::FLAG_USER_LIST));
	} 
	if(!showTree || isCurDir(dir)) {
		ii->update();
		if(noSort)
			ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, WinUtil::getIconIndex(Text::toT(ii->getTarget())));
		else
			ctrlQueue.insertItem(ii, WinUtil::getIconIndex(Text::toT(ii->getTarget())));
	}
}

QueueFrame::QueueItemInfo* QueueFrame::getItemInfo(const string& target) {
	string path = Util::getFilePath(target);
	DirectoryPair items = directories.equal_range(path);
	for(DirectoryIter i = items.first; i != items.second; ++i) {
		if(i->second->getTarget() == target) {
			return i->second;
		}
	}
	return 0;
}

void QueueFrame::addQueueList(const QueueItem::StringMap& li) {
	ctrlQueue.SetRedraw(FALSE);
	ctrlDirs.SetRedraw(FALSE);
	for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		QueueItemInfo* ii = new QueueItemInfo(*aQI);
		addQueueItem(ii, true);
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	ctrlDirs.SetRedraw(TRUE);
	ctrlDirs.Invalidate();
}

HTREEITEM QueueFrame::addDirectory(const string& dir, bool isFileList /* = false */, HTREEITEM startAt /* = NULL */) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvi.item.iImage = tvi.item.iSelectedImage = WinUtil::getDirIconIndex();

	if(isFileList) {
		// We assume we haven't added it yet, and that all filelists go to the same
		// directory...
		dcassert(fileLists == NULL);
		tvi.hParent = NULL;
		tvi.item.pszText = FILE_LIST_NAME;
		tvi.item.lParam = (LPARAM) new string(dir);
		fileLists = ctrlDirs.InsertItem(&tvi);
		return fileLists;
	} 

	// More complicated, we have to find the last available tree item and then see...
	string::size_type i = 0;
	string::size_type j;

	HTREEITEM next = NULL;
	HTREEITEM parent = NULL;

	if(startAt == NULL) {
		// First find the correct drive letter
		dcassert(dir[1] == ':');
		dcassert(dir[2] == '\\');

		next = ctrlDirs.GetRootItem();

		while(next != NULL) {
			if(next != fileLists) {
				string* stmp = reinterpret_cast<string*>(ctrlDirs.GetItemData(next));
					if(Util::strnicmp(*stmp, dir, 3) == 0)
						break;
				}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

	if(next == NULL) {
		// First addition, set commonStart to the dir minus the last part...
		i = dir.rfind('\\', dir.length()-2);
			if(i != string::npos) {
				tstring name = Text::toT(dir.substr(0, i));
			tvi.hParent = NULL;
				tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
				tvi.item.lParam = (LPARAM)new string(dir.substr(0, i+1));
				next = ctrlDirs.InsertItem(&tvi);
		} else {
				dcassert(dir.length() == 3);
				tstring name = Text::toT(dir);
				tvi.hParent = NULL;
				tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
				tvi.item.lParam = (LPARAM)new string(dir);
				next = ctrlDirs.InsertItem(&tvi);
			}
		} 
		
		// Ok, next now points to x:\... find how much is common

		string* rootStr = (string*)ctrlDirs.GetItemData(next);
		
			i = 0;

			for(;;) {
			j = dir.find('\\', i);
				if(j == string::npos)
					break;
				if(Util::strnicmp(dir.c_str() + i, rootStr->c_str() + i, j - i + 1) != 0)
					break;
					i = j + 1;
				}
		
		if(i < rootStr->length()) {
			HTREEITEM oldRoot = next;

			// Create a new root
			tstring name = Text::toT(rootStr->substr(0, i-1));
			tvi.hParent = NULL;
			tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
			tvi.item.lParam = (LPARAM)new string(rootStr->substr(0, i));
			HTREEITEM newRoot = ctrlDirs.InsertItem(&tvi);

			parent = addDirectory(*rootStr, false, newRoot);
                        while((next = ctrlDirs.GetChildItem(oldRoot)) != NULL) {
				moveNode(next, parent);
				}
                        
			ctrlDirs.DeleteItem(oldRoot);
                        delete rootStr; 

			parent = newRoot;
			} else {
			// Use this root as parent
			parent = next;
				next = ctrlDirs.GetChildItem(parent);
		}
	} else {
		parent = startAt;
		next = ctrlDirs.GetChildItem(parent);
		i = getDir(parent).length();
		dcassert(Util::strnicmp(getDir(parent), dir, getDir(parent).length()) == 0);
	}

	HTREEITEM firstParent = parent;

	while( i < dir.length() ) {
		while(next != NULL) {
			if(next != fileLists) {
				const string& n = getDir(next);
			if(Util::strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
				// Found a part, we assume it's the best one we can find...
				i = n.length();

				parent = next;
				next = ctrlDirs.GetChildItem(next);
				break;
			}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

		if(next == NULL) {
			// We didn't find it, add...
			j = dir.find('\\', i);
			dcassert(j != string::npos);
			tstring name = Text::toT(dir.substr(i, j-i));
			tvi.hParent = parent;
			tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
			tvi.item.lParam = (LPARAM) new string(dir.substr(0, j+1));
			
			parent = ctrlDirs.InsertItem(&tvi);
			
			i = j + 1;
		}
	}

	if(BOOLSETTING(EXPAND_QUEUE) && firstParent != NULL)
		ctrlDirs.Expand(firstParent);

	return parent;
}

void QueueFrame::removeDirectory(const string& dir, bool isFileList /* = false */) {

	// First, find the last name available
	string::size_type i = 0;

	HTREEITEM next = ctrlDirs.GetRootItem();
	HTREEITEM parent = NULL;
	
	if(isFileList) {
		dcassert(fileLists != NULL);
		delete (string*)ctrlDirs.GetItemData(fileLists);
		ctrlDirs.DeleteItem(fileLists);
		fileLists = NULL;
		return;
	} else {
		while(i < dir.length()) {
			while(next != NULL) {
				if(next != fileLists) {
				const string& n = getDir(next);
				if(Util::strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Match!
					parent = next;
					next = ctrlDirs.GetChildItem(next);
					i = n.length();
					break;
				}
				}
				next = ctrlDirs.GetNextSiblingItem(next);
			}
			if(next == NULL)
				break;
		}
	}

	next = parent;

	while((ctrlDirs.GetChildItem(next) == NULL) && (directories.find(getDir(next)) == directories.end())) {
		delete (string*)ctrlDirs.GetItemData(next);
		parent = ctrlDirs.GetParentItem(next);
		
		ctrlDirs.DeleteItem(next);
		if(parent == NULL)
			break;
		next = parent;
	}
}

void QueueFrame::removeDirectories(HTREEITEM ht) {
	HTREEITEM next = ctrlDirs.GetChildItem(ht);
	while(next != NULL) {
		removeDirectories(next);
		next = ctrlDirs.GetNextSiblingItem(ht);
	}
	delete (string*)ctrlDirs.GetItemData(ht);
	ctrlDirs.DeleteItem(ht);
}

void QueueFrame::on(QueueManagerListener::Removed, QueueItem* aQI) {
	speak(REMOVE_ITEM, new StringTask(aQI->getTarget()));
}

void QueueFrame::on(QueueManagerListener::Moved, QueueItem* aQI, const string& oldTarget) {
	speak(REMOVE_ITEM, new StringTask(oldTarget));
	speak(ADD_ITEM,	new QueueItemInfoTask(new QueueItemInfo(*aQI)));
}

void QueueFrame::on(QueueManagerListener::SourcesUpdated, QueueItem* aQI) {
	speak(UPDATE_ITEM, new UpdateTask(*aQI));
}

LRESULT QueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;

	tasks.get(t);
	spoken = false;

	for(TaskQueue::Iter ti = t.begin(); ti != t.end(); ++ti) {
		if(ti->first == ADD_ITEM) {
			auto_ptr<QueueItemInfoTask> iit(static_cast<QueueItemInfoTask*>(ti->second));
			
			dcassert(ctrlQueue.findItem(iit->ii) == -1);
			addQueueItem(iit->ii, false);
			updateStatus();
		} else if(ti->first == REMOVE_ITEM) {
			auto_ptr<StringTask> target(static_cast<StringTask*>(ti->second));
			QueueItemInfo* ii = getItemInfo(target->str);
			if(!ii) {
				dcassert(ii);
				continue;
			}
			
			if(!showTree || isCurDir(ii->getPath()) ) {
				dcassert(ctrlQueue.findItem(ii) != -1);
				ctrlQueue.deleteItem(ii);
			}
			
			if(!ii->isSet(QueueItem::FLAG_USER_LIST) && !ii->isSet(QueueItem::FLAG_TESTSUR)) {
				queueSize-=ii->getSize();
				dcassert(queueSize >= 0);
			}
			queueItems--;
			dcassert(queueItems >= 0);
			
			pair<DirectoryIter, DirectoryIter> i = directories.equal_range(ii->getPath());
			DirectoryIter j;
			for(j = i.first; j != i.second; ++j) {
				if(j->second == ii)
					break;
			}
			dcassert(j != i.second);
			directories.erase(j);
			if(directories.count(ii->getPath()) == 0) {
				removeDirectory(ii->getPath(), ii->isSet(QueueItem::FLAG_USER_LIST));
				if(isCurDir(ii->getPath()))
					curDir.clear();
			}
			
			delete ii;
			updateStatus();
			if (BOOLSETTING(BOLD_QUEUE)) {
				setDirty();
			}
			dirty = true;
		} else if(ti->first == UPDATE_ITEM) {
			auto_ptr<UpdateTask> ui(reinterpret_cast<UpdateTask*>(ti->second));
            QueueItemInfo* ii = getItemInfo(ui->target);
			if(!ii)
				continue;

			ii->setPriority(ui->priority);
			ii->setStatus(ui->status);
			ii->setDownloadedBytes(ui->downloadedBytes);
			ii->setSources(ui->sources);
			ii->setBadSources(ui->badSources);
			ii->setSize(ui->size);

			ii->updateMask |= QueueItemInfo::MASK_PRIORITY | QueueItemInfo::MASK_USERS | QueueItemInfo::MASK_ERRORS | QueueItemInfo::MASK_STATUS | QueueItemInfo::MASK_DOWNLOADED;

			if(!showTree || isCurDir(ii->getPath())) {
				dcassert(ctrlQueue.findItem(ii) != -1);
				ii->update();
				ctrlQueue.updateItem(ii);
			}
		}
	}

	return 0;
}

void QueueFrame::removeSelected() {
	if(!BOOLSETTING(CONFIRM_DELETE) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
	ctrlQueue.forEachSelected(&QueueItemInfo::remove);
}
	
void QueueFrame::removeSelectedDir() { 
	if(!BOOLSETTING(CONFIRM_DELETE) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)	
		removeDir(ctrlDirs.GetSelectedItem()); 
}

void QueueFrame::moveSelected() {

	int n = ctrlQueue.GetSelectedCount();
	if(n == 1) {
		// Single file, get the full filename and move...
		QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		tstring target = Text::toT(ii->getTarget());
		tstring ext = Util::getFileExt(target);
		tstring ext2;
		if (!ext.empty())
		{
			ext = ext.substr(1); // remove leading dot so default extension works when browsing for file
			ext2 = _T("*.") + ext;
			ext2 += (TCHAR)0;
			ext2 += _T("*.") + ext;
		}
		ext2 += _T("*.*");
		ext2 += (TCHAR)0;
		ext2 += _T("*.*");
		ext2 += (TCHAR)0;

		tstring path = Text::toT(ii->getPath());
		if(WinUtil::browseFile(target, m_hWnd, true, path, ext2.c_str(), ext.empty() ? NULL : ext.c_str())) {
			QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(target));
		}
	} else if(n > 1) {
		tstring name;
		if(showTree) {
			name = Text::toT(curDir);
		}

		if(WinUtil::browseDirectory(name, m_hWnd)) {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				QueueItemInfo* ii = ctrlQueue.getItemData(i);
				QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(name) + Util::getFileName(ii->getTarget()));
			}			
		}
	}
}

void QueueFrame::moveSelectedDir() {
	if(ctrlDirs.GetSelectedItem() == NULL)
		return;

	dcassert(!curDir.empty());
	tstring name = Text::toT(curDir);
	
	if(WinUtil::browseDirectory(name, m_hWnd)) {
		moveDir(ctrlDirs.GetSelectedItem(), Text::fromT(name));
	}
}

void QueueFrame::moveDir(HTREEITEM ht, const string& target) {
	HTREEITEM next = ctrlDirs.GetChildItem(ht);
	while(next != NULL) {
		moveDir(next, target + Util::getLastDir(getDir(next)));
		next = ctrlDirs.GetNextSiblingItem(next);
	}
	string* s = (string*)ctrlDirs.GetItemData(ht);

	DirectoryPair p = directories.equal_range(*s);
	
	for(DirectoryIter i = p.first; i != p.second; ++i) {
		QueueItemInfo* ii = i->second;
		QueueManager::getInstance()->move(ii->getTarget(), target + Util::getFileName(ii->getTarget()));
	}			
}

LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
                // !BUGMASTER!-S
                for(int i=0;i<14;++i) {
			segmentsMenu.CheckMenuItem(i, MF_BYPOSITION | MF_UNCHECKED);
                        segmentsMenu.EnableMenuItem(i, MF_BYPOSITION | MF_ENABLED);
		}

		for(int i=0;i<7;i++)
			priorityMenu.CheckMenuItem(i, MF_BYPOSITION | MF_UNCHECKED);
			
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
		}
			
		if(ctrlQueue.GetSelectedCount() > 0) { 
			usingDirMenu = false;
			CMenuItemInfo mi;
		
			while(browseMenu.GetMenuItemCount() > 0) {
				browseMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(removeMenu.GetMenuItemCount() > 2) {
				removeMenu.RemoveMenu(2, MF_BYPOSITION);
			}
			while(removeAllMenu.GetMenuItemCount() > 0) {
				removeAllMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(pmMenu.GetMenuItemCount() > 0) {
				pmMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(readdMenu.GetMenuItemCount() > 2) {
				readdMenu.RemoveMenu(2, MF_BYPOSITION);
			}

			while(previewMenu.GetMenuItemCount() > 0) {
				previewMenu.RemoveMenu(0, MF_BYPOSITION);
			}		

		if(ctrlQueue.GetSelectedCount() == 1) {
			QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
			QueueItem* qi = QueueManager::getInstance()->fileQueue.find(ii->getTarget());
			if(!qi) return 0;

                        // !BUGMASTER!-S
                        segmentsMenu.CheckMenuItem(IDC_SEGMENTONE-1+qi->getMaxSegments(), MF_CHECKED);

			if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
                                // !BUGMASTER!-S
                                segmentsMenu.EnableMenuItem(0, MF_BYPOSITION | MFS_DISABLED);
			} else {
                                // !BUGMASTER!-S
                                for(int i=1;i<14;++i) {
                                        segmentsMenu.EnableMenuItem(i, MF_BYPOSITION | MFS_DISABLED);
				}
			}
			if((ii->isSet(QueueItem::FLAG_USER_LIST)) == false) {
				string ext = Util::getFileExt(ii->getTarget());
				if(ext.size()>1) ext = ext.substr(1);
				PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);
				if(previewMenu.GetMenuItemCount() > 0) {
					singleMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
				} else {
					singleMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
				}
				previewMenu.InsertSeparatorFirst(TSTRING(PREVIEW_MENU));
			}

			menuItems = 0;
			int pmItems = 0;
			if(ii) {
				for(QueueItem::SourceIter i = ii->getSources().begin(); i != ii->getSources().end(); ++i) {
					tstring l_hub =  WinUtil::getHubNames(i->getUser()).first; //[+]PPA
					tstring nick = WinUtil::escapeMenu(Text::toT(i->getUser()->getFirstNick() + string(" - ")) +  l_hub); //[+]PPA
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.dwTypeData = (LPTSTR)nick.c_str();
					mi.dwItemData = (ULONG_PTR)&(*i);
					mi.wID = IDC_BROWSELIST + menuItems;
					browseMenu.InsertMenuItem(menuItems, TRUE, &mi);
					mi.wID = IDC_REMOVE_SOURCE + 1 + menuItems; // "All" is before sources
					removeMenu.InsertMenuItem(menuItems + 2, TRUE, &mi); // "All" and separator come first
					mi.wID = IDC_REMOVE_SOURCES + menuItems;
					removeAllMenu.InsertMenuItem(menuItems, TRUE, &mi);
					if(i->getUser()->isOnline()) {
						mi.wID = IDC_PM + menuItems;
						pmMenu.InsertMenuItem(menuItems, TRUE, &mi);
						pmItems++;
					}
					menuItems++;
				}
				readdItems = 0;
				for(QueueItem::SourceIter i = ii->getBadSources().begin(); i != ii->getBadSources().end(); ++i) {
  				    tstring l_hub =  WinUtil::getHubNames(i->getUser()).first; //[+]PPA
					tstring nick = WinUtil::escapeMenu(Text::toT(i->getUser()->getFirstNick() + string(" - ")) +  l_hub); //[+]PPA
					if(i->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						nick += _T(" (") + TSTRING(FILE_NOT_AVAILABLE) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_PASSIVE)) {
						nick += _T(" (") + TSTRING(PASSIVE_USER) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_ROLLBACK_INCONSISTENCY)) {
						nick += _T(" (") + TSTRING(ROLLBACK_INCONSISTENCY) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						nick += _T(" (") + TSTRING(INVALID_TREE) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_SLOW)) {
						nick += _T(" (") + TSTRING(SLOW_USER) + _T(" [") + Util::toStringW(i->getUser()->getLastDownloadSpeed()) + _T(" kB/s])");
					} else if(i->isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
						nick += _T(" (") + TSTRING(NO_NEEDED_PART) + _T(")");
					} else if(i->isSet(QueueItem::Source::FLAG_NO_TTHF)) {
						nick += _T(" (") + TSTRING(SOURCE_TOO_OLD) + _T(")");
					}					
					mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
					mi.fType = MFT_STRING;
					mi.dwTypeData = (LPTSTR)nick.c_str();
					mi.dwItemData = (ULONG_PTR)&(*i);
					mi.wID = IDC_READD + 1 + readdItems;  // "All" is before sources
					readdMenu.InsertMenuItem(readdItems + 2, TRUE, &mi);  // "All" and separator come first
					readdItems++;
				}
			}

			if(menuItems == 0) {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)browseMenu, MFS_DISABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeMenu, MFS_DISABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeAllMenu, MFS_DISABLED);
			}
			else {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)browseMenu, MFS_ENABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeMenu, MFS_ENABLED);
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeAllMenu, MFS_ENABLED);
			}

			if(pmItems == 0) {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)pmMenu, MFS_DISABLED);
			}
			else {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)pmMenu, MFS_ENABLED);
			}

			if(readdItems == 0) {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)readdMenu, MFS_DISABLED);
			}
			else {
				singleMenu.EnableMenuItem((UINT_PTR)(HMENU)readdMenu, MFS_ENABLED);
 			}

			UINT pos = 0;
			switch(ii->getPriority()) {
				case QueueItem::PAUSED: pos = 0; break;
				case QueueItem::LOWEST: pos = 1; break;
				case QueueItem::LOW: pos = 2; break;
				case QueueItem::NORMAL: pos = 3; break;
				case QueueItem::HIGH: pos = 4; break;
				case QueueItem::HIGHEST: pos = 5; break;
				default: dcassert(0); break;
			}
			priorityMenu.CheckMenuItem(pos, MF_BYPOSITION | MF_CHECKED);
			if(ii->getAutoPriority())
				priorityMenu.CheckMenuItem(6, MF_BYPOSITION | MF_CHECKED);

				browseMenu.InsertSeparatorFirst(TSTRING(GET_FILE_LIST));
				removeMenu.InsertSeparatorFirst(TSTRING(REMOVE_SOURCE));
				removeAllMenu.InsertSeparatorFirst(TSTRING(REMOVE_FROM_ALL));
				pmMenu.InsertSeparatorFirst(TSTRING(SEND_PRIVATE_MESSAGE));
				readdMenu.InsertSeparatorFirst(TSTRING(READD_SOURCE));
				segmentsMenu.InsertSeparatorFirst(TSTRING(MAX_SEGMENTS_NUMBER));
				priorityMenu.InsertSeparatorFirst(TSTRING(PRIORITY));

				singleMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

				segmentsMenu.RemoveFirstItem();
				priorityMenu.RemoveFirstItem();
				browseMenu.RemoveFirstItem();
				removeMenu.RemoveFirstItem();
				removeAllMenu.RemoveFirstItem();
				pmMenu.RemoveFirstItem();
				readdMenu.RemoveFirstItem();
			} else {
				priorityMenu.InsertSeparatorFirst(TSTRING(PRIORITY));
				multiMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
				priorityMenu.RemoveFirstItem();
			}
		
			return TRUE; 
		}
	} else if (reinterpret_cast<HWND>(wParam) == ctrlDirs && ctrlDirs.GetSelectedItem() != NULL) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlDirs, pt);
		} else {
			// Strange, windows doesn't change the selection on right-click... (!)
			UINT a = 0;
			ctrlDirs.ScreenToClient(&pt);
			HTREEITEM ht = ctrlDirs.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlDirs.GetSelectedItem())
				ctrlDirs.SelectItem(ht);
			ctrlDirs.ClientToScreen(&pt);
		}
		usingDirMenu = true;
		
		priorityMenu.InsertSeparatorFirst(TSTRING(PRIORITY));
		dirMenu.InsertSeparatorFirst(TSTRING(FOLDER));
		dirMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		priorityMenu.RemoveFirstItem();
		dirMenu.RemoveFirstItem();
	
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT QueueFrame::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        // !SMT!-UI (multiple search)
        int i = -1;
        while ((i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1)
                WinUtil::searchHash(ctrlQueue.getItemData(i)->getTTH());
        return 0;
}

LRESULT QueueFrame::onBrowseList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		browseMenu.GetMenuItemInfo(wID, FALSE, &mi);
		OMenuItem* omi = (OMenuItem*)mi.dwItemData;
		QueueItem::Source* s = (QueueItem::Source*)omi->data;
		try {
			QueueManager::getInstance()->addList(s->getUser(), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception&) {
		}
	}
	return 0;
}

LRESULT QueueFrame::onReadd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		QueueItemInfo* ii = ctrlQueue.getItemData(i);

		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		readdMenu.GetMenuItemInfo(wID, FALSE, &mi);
                if(wID == IDC_READD) {
                        // re-add all sources
                        for(QueueItem::SourceIter s = ii->getBadSources().begin(); s != ii->getBadSources().end(); s++) {
                                QueueManager::getInstance()->readd(ii->getTarget(), s->getUser());
                        }
		} else {
			OMenuItem* omi = (OMenuItem*)mi.dwItemData;
			QueueItem::Source* s = (QueueItem::Source*)omi->data;
			try {
				QueueManager::getInstance()->readd(ii->getTarget(), s->getUser());
			} catch(const Exception& e) {
				ctrlStatus.SetText(0, Text::toT(e.getError()).c_str());
			}
		}
	}
	return 0;
}

LRESULT QueueFrame::onRemoveSource(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

        if(ctrlQueue.GetSelectedCount() == 1) {
                int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
                QueueItemInfo* ii = ctrlQueue.getItemData(i);
                if(wID == IDC_REMOVE_SOURCE) {
                        for(QueueItem::SourceIter si = ii->getSources().begin(); si != ii->getSources().end(); si++) {
                                QueueManager::getInstance()->removeSource(ii->getTarget(), si->getUser(), QueueItem::Source::FLAG_REMOVED);
                        }
		} else {
			CMenuItemInfo mi;
			mi.fMask = MIIM_DATA;

			removeMenu.GetMenuItemInfo(wID, FALSE, &mi);
			OMenuItem* omi = (OMenuItem*)mi.dwItemData;
			QueueItem::Source* s = (QueueItem::Source*)omi->data;
			QueueManager::getInstance()->removeSource(ii->getTarget(), s->getUser(), QueueItem::Source::FLAG_REMOVED);
		}
	}
	return 0;
}

LRESULT QueueFrame::onRemoveSources(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CMenuItemInfo mi;
	mi.fMask = MIIM_DATA;
	removeAllMenu.GetMenuItemInfo(wID, FALSE, &mi);
	OMenuItem* omi = (OMenuItem*)mi.dwItemData;
	QueueItem::Source* s = (QueueItem::Source*)omi->data;
	QueueManager::getInstance()->removeSource(s->getUser(), QueueItem::Source::FLAG_REMOVED);
	return 0;
}

LRESULT QueueFrame::onPM(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		pmMenu.GetMenuItemInfo(wID, FALSE, &mi);
		OMenuItem* omi = (OMenuItem*)mi.dwItemData;
		QueueItem::Source* s = (QueueItem::Source*)omi->data;
		PrivateFrameFactory::openWindow(s->getUser());
	}
	return 0;
}

LRESULT QueueFrame::onAutoPriority(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	

	if(usingDirMenu) {
		setAutoPriority(ctrlDirs.GetSelectedItem(), true);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setAutoPriority(ctrlQueue.getItemData(i)->getTarget(),!ctrlQueue.getItemData(i)->getAutoPriority());
		}
	}
	return 0;
}

LRESULT QueueFrame::onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		QueueItem* qi = QueueManager::getInstance()->fileQueue.find(ii->getTarget());
		if(qi && qi->isSet(QueueItem::FLAG_MULTI_SOURCE))
                        qi->setMaxSegments(max((uint8_t)2, (uint8_t)(wID - IDC_SEGMENTONE + 1))); // !BUGMASTER!-S
		ii->update();
		ctrlQueue.updateItem(ii);
	}

	return 0;
}

LRESULT QueueFrame::onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueItem::Priority p;

	switch(wID) {
		case IDC_PRIORITY_PAUSED: p = QueueItem::PAUSED; break;
		case IDC_PRIORITY_LOWEST: p = QueueItem::LOWEST; break;
		case IDC_PRIORITY_LOW: p = QueueItem::LOW; break;
		case IDC_PRIORITY_NORMAL: p = QueueItem::NORMAL; break;
		case IDC_PRIORITY_HIGH: p = QueueItem::HIGH; break;
		case IDC_PRIORITY_HIGHEST: p = QueueItem::HIGHEST; break;
		default: p = QueueItem::DEFAULT; break;
	}

	if(usingDirMenu) {
		setPriority(ctrlDirs.GetSelectedItem(), p);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setAutoPriority(ctrlQueue.getItemData(i)->getTarget(), false);
			QueueManager::getInstance()->setPriority(ctrlQueue.getItemData(i)->getTarget(), p);
		}
	}

	return 0;
}

void QueueFrame::removeDir(HTREEITEM ht) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		removeDir(child);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	StringList l_tmp_target;
	for (DirectoryIter i = dp.first; i != dp.second; ++i)
		l_tmp_target.push_back(i->second->getTarget());
	for (StringList::const_iterator i = l_tmp_target.begin(); i != l_tmp_target.end(); ++i)
		QueueManager::getInstance()->remove(*i);
}

/*
 * @param inc True = increase, False = decrease
 */
void QueueFrame::changePriority(bool inc){
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1){
		QueueItem::Priority p = ctrlQueue.getItemData(i)->getPriority();

		if ((inc && p == QueueItem::HIGHEST) || (!inc && p == QueueItem::PAUSED)){
			// Trying to go higher than HIGHEST or lower than PAUSED
			// so do nothing
			continue;
		}

		switch(p){
			case QueueItem::HIGHEST: p = QueueItem::HIGH; break;
			case QueueItem::HIGH:    p = inc ? QueueItem::HIGHEST : QueueItem::NORMAL; break;
			case QueueItem::NORMAL:  p = inc ? QueueItem::HIGH    : QueueItem::LOW; break;
			case QueueItem::LOW:     p = inc ? QueueItem::NORMAL  : QueueItem::LOWEST; break;
			case QueueItem::LOWEST:  p = inc ? QueueItem::LOW     : QueueItem::PAUSED; break;
			case QueueItem::PAUSED:  p = QueueItem::LOWEST; break;
		}

		QueueManager::getInstance()->setPriority(ctrlQueue.getItemData(i)->getTarget(), p);
	}
}

void QueueFrame::setPriority(HTREEITEM ht, const QueueItem::Priority& p) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setPriority(child, p);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setPriority(i->second->getTarget(), p);
	}
}

void QueueFrame::setAutoPriority(HTREEITEM ht, const bool& ap) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setAutoPriority(child, ap);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPair dp = directories.equal_range(name);
	for(DirectoryIter i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setAutoPriority(i->second->getTarget(), ap);
	}
}

void QueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {
		int64_t total = 0;
		int cnt = ctrlQueue.GetSelectedCount();
		if(cnt == 0) {
			cnt = ctrlQueue.GetItemCount();
			if(showTree) {
				for(int i = 0; i < cnt; ++i) {
					QueueItemInfo* ii = ctrlQueue.getItemData(i);
					total += (ii->getSize() > 0) ? ii->getSize() : 0;
				}
			} else {
				total = queueSize;
			}
		} else {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				QueueItemInfo* ii = ctrlQueue.getItemData(i);
				total += (ii->getSize() > 0) ? ii->getSize() : 0;
			}

		}

		tstring tmp1 = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
		tstring tmp2 = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(total);
		bool u = false;

		int w = WinUtil::getTextWidth(tmp1, ctrlStatus.m_hWnd);
		if(statusSizes[1] < w) {
			statusSizes[1] = w;
			u = true;
		}
		ctrlStatus.SetText(2, tmp1.c_str());
		w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
		if(statusSizes[2] < w) {
			statusSizes[2] = w;
			u = true;
		}
		ctrlStatus.SetText(3, tmp2.c_str());

		if(dirty) {
			tmp1 = TSTRING(FILES) + _T(": ") + Util::toStringW(queueItems);
			tmp2 = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(queueSize);

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[3] < w) {
				statusSizes[3] = w;
				u = true;
			}
			ctrlStatus.SetText(4, tmp1.c_str());

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[4] < w) {
				statusSizes[4] = w;
				u = true;
			}
			ctrlStatus.SetText(5, tmp2.c_str());

			dirty = false;
		}

		if(u)
			UpdateLayout(TRUE);
	}
}

void QueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
        m_header.updateLayout(rect.left, rect.top, rect.right);
        int buttonStartX = m_header.getLeftMargin() + m_header.getTextWidth() + H_GAP;
        m_btnPause.MoveWindow(buttonStartX, (m_header.getPreferredHeight() - BUTTON_HEIGHT) / 2, BUTTON_WIDTH, BUTTON_HEIGHT);
        buttonStartX += BUTTON_WIDTH + H_GAP;
        m_btnDelete.MoveWindow(buttonStartX, (m_header.getPreferredHeight() - BUTTON_HEIGHT) / 2, BUTTON_WIDTH, BUTTON_HEIGHT);
        buttonStartX += BUTTON_WIDTH + H_GAP * 5;
        m_prioritySlider.MoveWindow(buttonStartX, (m_header.getPreferredHeight() - m_prioritySlider.getPreferredHeight()) / 2, m_prioritySlider.getPreferredWidth(), m_prioritySlider.getPreferredHeight());
        buttonStartX += m_prioritySlider.getPreferredWidth() + H_GAP;
        SIZE size = ControlAdjuster::adjustStaticSize(m_priorityLabel);
        m_priorityLabel.MoveWindow(buttonStartX, (m_header.getPreferredHeight() - size.cy) / 2, size.cx, size.cy);
        buttonStartX += size.cx + H_GAP;
        size = ControlAdjuster::adjustStaticSize(m_priorityValue);
        m_priorityValue.MoveWindow(buttonStartX, (m_header.getPreferredHeight() - size.cy) / 2, size.cx, size.cy);
        rect.top += m_header.getPreferredHeight();

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);
		w[5] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(4); setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}

	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
			updateQueue();
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
			updateQueue();
		}
	}
	
	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  QueueManager::getInstance()->removeListener(this);
  SettingsManager::getInstance()->removeListener(this);
  WinUtil::setButtonPressed(IDC_QUEUE, false);
  HTREEITEM ht = ctrlDirs.GetRootItem();
  while(ht != NULL) {
    clearTree(ht);
    ht = ctrlDirs.GetNextSiblingItem(ht);
  }

  SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
  for(DirectoryIter i = directories.begin(); i != directories.end(); ++i) {
    delete i->second;
  }
  directories.clear();
  ctrlQueue.DeleteAllItems();

  ctrlQueue.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER, 
    SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);

  bHandled = FALSE;
  return 0;
}

LRESULT QueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	updateQueue();
	return 0;
}

void QueueFrame::onTab() {
	if(showTree) {
		HWND focus = ::GetFocus();
		if(focus == ctrlDirs.m_hWnd) {
			ctrlQueue.SetFocus();
		} else if(focus == ctrlQueue.m_hWnd) {
			ctrlDirs.SetFocus();
		}
	}
}

void QueueFrame::updateQueue() {
	ctrlQueue.DeleteAllItems();
	pair<DirectoryIter, DirectoryIter> i;
	if(showTree) {
		i = directories.equal_range(getSelectedDir());
	} else {
		i.first = directories.begin();
		i.second = directories.end();
	}

	ctrlQueue.SetRedraw(FALSE);
	for(DirectoryIter j = i.first; j != i.second; ++j) {
		QueueItemInfo* ii = j->second;
		ii->update();
		ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, WinUtil::getIconIndex(Text::toT(ii->getTarget())));
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	curDir = getSelectedDir();
	updateStatus();
}

void QueueFrame::clearTree(HTREEITEM item) {
	HTREEITEM next = ctrlDirs.GetChildItem(item);
	while(next != NULL) {
		clearTree(next);
		next = ctrlDirs.GetNextSiblingItem(next);
	}
	delete (string*)ctrlDirs.GetItemData(item);
}

void QueueFrame::moveNode(HTREEITEM item, HTREEITEM parent) {
	TVINSERTSTRUCT tvis;
	memzero(&tvis, sizeof(tvis));
	tvis.itemex.hItem = item;
        tvis.itemex.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_PARAM |
		TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;
        TCHAR tmpBuf[1024];
        tvis.item.pszText = tmpBuf;
        tvis.item.cchTextMax = 1024;
        ctrlDirs.GetItem(&tvis.item);
	tvis.hInsertAfter =	TVI_SORT;
	tvis.hParent = parent;
        tvis.item.mask &= ~TVIF_HANDLE; // !SMT!-F
        HTREEITEM newRoot = ctrlDirs.InsertItem(&tvis);
        for (;;) {
	HTREEITEM next = ctrlDirs.GetChildItem(item);
                if (!next) break;
                moveNode(next, newRoot);
	}
	ctrlDirs.DeleteItem(item);
}
	
LRESULT QueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	CRect rc;
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			QueueItemInfo *ii = (QueueItemInfo*)cd->nmcd.lItemlParam;
			if(ii->getText(COLUMN_ERRORS) != TSTRING(NO_ERRORS)) {
				cd->clrText = SETTING(ERROR_COLOR);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}				
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {

		int colIndex = ctrlQueue.findColumn(cd->iSubItem);
		if(colIndex == COLUMN_PROGRESS) {
			if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
				bHandled = FALSE;
				return 0;
			}			
			// draw something nice...
			ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			CRect real_rc = rc;
			rc.MoveToXY(0, 0);
			
			CRect rc2 = rc;
                        rc2.left += 6; // indented with 6 pixels
			rc2.right -= 2; // and without messing with the border of the cell				

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());
			HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
			HDC& dc = cdc.m_hDC;

			SetBkMode(dc, TRANSPARENT);
		
			QueueItemInfo *qi = (QueueItemInfo*)cd->nmcd.lItemlParam;
                        CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, SETTING(PROGRESS_BACK_COLOR), (uint64_t)max((int64_t)1, (int64_t)qi->getSize()));

			if(qi->chunkInfo) {
				vector<int64_t> v;

				// avoiding chunks - first find the highest aversion score
				qi->chunkInfo->getAllChunks(v, 3);
				//for this the vector comes in threes, with the third element being the aversion
				int64_t maxAversion = 1; // start with a fictitious maximum of 1 to prevent division by 0
				for(vector<int64_t>::const_iterator i = v.begin(); i < v.end(); i += 3) {
					if(*(i+2) > maxAversion) {
						maxAversion = *(i+2);
					}
				}

				for(vector<int64_t>::const_iterator i = v.begin(); i < v.end(); i += 3) {
					//calculate the offset colour that this chunk would be
					COLORREF crThisAverse = (COLORREF)(SETTING(COLOR_AVOIDING) * *(i+2)/maxAversion);
					//but then add this to the background, so it fades, rather than just showing it
					statusBar.FillRange(*i, *(i+1), crThisAverse + SETTING(PROGRESS_BACK_COLOR));
				}
				v.clear();

				// running chunks
				qi->chunkInfo->getAllChunks(v, 1);
				for(vector<int64_t>::const_iterator i = v.begin(); i < v.end(); i += 2) {
                                        statusBar.FillRange(*i, *(i+1), SETTING(COLOR_RUNNING));
				}
				v.clear();

				// downloaded chunks
				qi->chunkInfo->getAllChunks(v, 0);
				for(vector<int64_t>::const_iterator i = v.begin(); i < v.end(); i += 2) {
                                        statusBar.FillRange(*i, *(i+1), SETTING(COLOR_DOWNLOADED));
				}
				v.clear();

				// verified chunks
				qi->chunkInfo->getAllChunks(v, 2);
				for(vector<int64_t>::const_iterator i = v.begin(); i < v.end(); i += 2) {
                                        statusBar.FillRange(*i, *(i+1), SETTING(COLOR_VERIFIED));
				}
			} else {
				int64_t possibleVerified = qi->getDownloadedBytes() - (qi->getDownloadedBytes() % 65536);
                                statusBar.FillRange(0, possibleVerified, SETTING(COLOR_VERIFIED));
                                statusBar.FillRange(possibleVerified, qi->getDownloadedBytes(), SETTING(COLOR_DOWNLOADED));
			}
			statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));
			BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			return CDRF_SKIPDEFAULT;
		}
	}
	default:
		return CDRF_DODEFAULT;
	}
}			

LRESULT QueueFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        tstring data;
        int i = -1, columnId = wID-IDC_COPY; // !SMT!-UI: copy several rows
        while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
                QueueItemInfo* ii = ctrlQueue.getItemData(i);
                tstring sdata;
                // !SMT!-UI
                if (wID == IDC_COPY_LINK)
                        sdata = Text::toT(WinUtil::getMagnet(ii->getTTH(), Text::toT(Util::getFileName(ii->getTarget())), ii->getSize()));
                else if (wID == IDC_COPY_WMLINK)
                        sdata = Text::toT(WinUtil::getWebMagnet(ii->getTTH(), Text::toT(Util::getFileName(ii->getTarget())), ii->getSize()));
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

LRESULT QueueFrame::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	if(ctrlQueue.GetSelectedCount() == 1) {
		QueueItemInfo* i = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		QueueItem* qi = QueueManager::getInstance()->fileQueue.find(i->getTarget());
		if(qi)
			WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, qi->getTempTarget());
	}
	return 0;
}

LRESULT QueueFrame::onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		QueueItemInfo* ii = ctrlQueue.getItemData(i);

                for(QueueItem::SourceIter i = ii->getSources().begin(); i != ii->getSources().end(); i++) {
                        if(!i->getUser()->isOnline()) {
                                QueueManager::getInstance()->removeSource(ii->getTarget(), i->getUser(), QueueItem::Source::FLAG_REMOVED);
                        }
                }
	}
	return 0;
}

void QueueFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlQueue.GetBkColor() != WinUtil::bgColor) {
		ctrlQueue.SetBkColor(WinUtil::bgColor);
		ctrlQueue.SetTextBkColor(WinUtil::bgColor);
		ctrlQueue.setFlickerFree(WinUtil::bgBrush);
		ctrlDirs.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlQueue.GetTextColor() != WinUtil::textColor) {
		ctrlQueue.SetTextColor(WinUtil::textColor);
		ctrlDirs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT QueueFrame::onIdle(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  const bool enableButtons = ctrlQueue.GetSelectedCount() > 0;
  bool hasPaused = false;
  QueueItem::Priority minPriority = QueueItem::LAST;
  if (enableButtons) {
    for (Selection i(ctrlQueue); i.hasNext(); ) {
      const QueueItemInfo* qi = i.next();
      if (qi->getPriority() == QueueItem::PAUSED) {
        hasPaused = true;
      }
      if (qi->getPriority() < minPriority) {
        minPriority = qi->getPriority();
      }
    }
  }
  m_btnPause.EnableWindow(enableButtons);
  if (m_btnPause.setIconIndex(hasPaused ? 1 : 0)) {
    m_btnPause.setHintText(WSTRING_I(hasPaused ? ResourceManager::DOWNLOAD_START_HINT : ResourceManager::DOWNLOAD_PAUSE_HINT));
  }
  m_btnDelete.EnableWindow(enableButtons);
  m_prioritySlider.EnableWindow(enableButtons);
  m_priorityLabel.EnableWindow(enableButtons);
  if (enableButtons) {
    m_prioritySlider.setPriority(minPriority);
    if (m_priorityValue.GetWindowLong(GWL_USERDATA) != minPriority) {
      m_priorityValue.SetWindowText(getPriorityName(minPriority));
      m_priorityValue.SetWindowLong(GWL_USERDATA, minPriority);
      const SIZE size = ControlAdjuster::adjustStaticSize(m_priorityValue);
      m_priorityValue.MoveWindow(ControlAdjuster::getWindowOrigin(m_priorityValue).x, (m_header.getPreferredHeight() - size.cy) / 2, size.cx, size.cy);
    }
  }
  else {
    if (m_priorityValue.GetWindowLong(GWL_USERDATA) != -1) {
      m_priorityValue.SetWindowText(_T(""));
      m_priorityValue.SetWindowLong(GWL_USERDATA, -1);
    }
  }
  return 0;
}

LRESULT QueueFrame::onStartPauseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (ctrlQueue.GetSelectedCount() > 0) {
    bool hasPaused = false;
    for (Selection i(ctrlQueue); i.hasNext();) {
      const QueueItemInfo* qi = i.next();
      if (qi->getPriority() == QueueItem::PAUSED) {
        hasPaused = true;
        break;
      }
    }
    QueueManager* const qm = QueueManager::getInstance();
    for (Selection i(ctrlQueue); i.hasNext();) {
      const QueueItemInfo* qi = i.next();
      if (hasPaused) {
        if (qi->getPriority() == QueueItem::PAUSED) {
          qm->setAutoPriority(qi->getTarget(), false);
          qm->setPriority(qi->getTarget(), QueueItem::NORMAL);
        }
      }
      else {
        if (qi->getPriority() != QueueItem::PAUSED) {
          qm->setAutoPriority(qi->getTarget(), false);
          qm->setPriority(qi->getTarget(), QueueItem::PAUSED);
        }
      }
    }
  }
  return 0;
}

LRESULT QueueFrame::onDeleteButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (ctrlQueue.GetSelectedCount() > 0) {
    QueueManager* const qm = QueueManager::getInstance();
    for (Selection i(ctrlQueue); i.hasNext();) {
      const QueueItemInfo* qi = i.next();
      qm->remove(qi->getTarget());
    }
  }
  return 0;
}

const TCHAR* QueueFrame::getPriorityName(QueueItem::Priority priority) const {
  switch (priority)
  {
  case QueueItem::PAUSED:   return CTSTRING(PAUSED);
  case QueueItem::LOWEST:   return CTSTRING(LOWEST);
  case QueueItem::LOW:      return CTSTRING(LOW);
  case QueueItem::NORMAL:   return CTSTRING(NORMAL);
  case QueueItem::HIGH:     return CTSTRING(HIGH);
  case QueueItem::HIGHEST:  return CTSTRING(HIGHEST);
  default:                  dcassert(false); return NULL;
  }
}

/**
 * @file
 * $Id: QueueFrame.cpp,v 1.11 2008/03/10 06:55:27 alexey Exp $
 */
