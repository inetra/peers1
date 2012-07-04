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

#include "SearchFrm.h"
#include "../peers/ControlAdjuster.h"

#include "../client/QueueManager.h"
#include "../client/ClientManager.h"
#include "../client/PGLoader.h"
#include "../client/ShareManager.h"
#include "../peers/PeersSearchBar.h"

#define FILTER_LENGTH 20
#define FILTER_RIGHT_MARGIN 8

int SearchFrame::columnIndexes[] = { 
  COLUMN_FILENAME,
  COLUMN_LOCAL_PATH, 
  COLUMN_HITS, 
  COLUMN_NICK,
  COLUMN_TYPE,
  COLUMN_SIZE,
  COLUMN_PATH,
  COLUMN_SLOTS,
  COLUMN_HUB, 
  COLUMN_EXACT_SIZE, 
  COLUMN_LOCATION,
  COLUMN_IP, 
#ifdef PPA_INCLUDE_DNS
  COLUMN_DNS, // !SMT!-IP
#endif
  COLUMN_TTH 
};

int SearchFrame::columnSizes[] = {
  210,
  //70,
  80, 
  100, 
  50, 
  80, 
  100, 
  40, 
  150, 
  80, 
  80,
  100, 
  100,
#ifdef PPA_INCLUDE_DNS
  100,
#endif
  150 
}; // !SMT!-IP

static ResourceManager::Strings columnNames[] = {
  ResourceManager::FILE, 
  ResourceManager::LOCAL_PATH,
  ResourceManager::HIT_COUNT, 
  ResourceManager::USER, 
  ResourceManager::TYPE, 
  ResourceManager::SIZE,
  ResourceManager::PATH, 
  ResourceManager::SLOTS,
  //[-]PPA ResourceManager::CONNECTION, 
  ResourceManager::HUB, 
  ResourceManager::EXACT_SIZE,
  //[-]PPA ResourceManager::AVERAGE_UPLOAD,
  ResourceManager::LOCATION_BARE,
  ResourceManager::IP_BARE,
#ifdef PPA_INCLUDE_DNS
  ResourceManager::DNS_BARE, // !SMT!-IP
#endif
  ResourceManager::TTH_ROOT 
};

SearchFrame::SearchFrame(): 
m_initialWordCount(0),
windowActivationCount(0),
initialSize(0),
initialMode(SearchManager::SIZE_ATLEAST),
initialType(SearchManager::TYPE_ANY),
onlyFree(false),
useGrouping(true),
isHash(false),
droppedResults(0),
resultsCount(0),
expandSR(false),
exactSize1(false),
exactSize2(0),
searches(0)
{	
  SearchManager::getInstance()->addListener(this);
  //useGrouping = BOOLSETTING(GROUP_SEARCH_RESULTS);
}

void SearchFrame::initialize(const SearchFrameFactory::Request& request) {
  initialString = request.str; 
  initialSize = request.size; 
  initialMode = request.mode; 
  initialType = request.type; 
  bPaused = false;
}

SearchFrame::~SearchFrame() {
  images.Destroy();
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  m_header.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
  m_header.addWords(_T("Результаты поиска"), m_header.captionFont, GetSysColor(COLOR_WINDOWTEXT));
  m_initialWordCount = m_header.size();

  const bool useSystemIcons = BOOLSETTING(USE_SYSTEM_ICONS);
  DWORD lvStyles = WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS;
  if (useSystemIcons) {
    lvStyles |= LVS_SHAREIMAGELISTS;
  }
  ctrlResults.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | lvStyles, WS_EX_CLIENTEDGE, IDC_RESULTS);
  ctrlResults.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

  if (BOOLSETTING(USE_SYSTEM_ICONS)) {
    ctrlResults.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
  } else {
    images.CreateFromImage(IDB_SPEEDS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
    ctrlResults.SetImageList(images, LVSIL_SMALL);
  }

  ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE, IDC_SEARCH_FILTER);
  ctrlFilter.SetFont(WinUtil::font);

  ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE, IDC_SEARCH_FILTER_MODE);
  ctrlFilterSel.SetFont(WinUtil::font);

  srLabel.Create(m_hWnd, rcDefault, CTSTRING(SEARCH_IN_RESULTS), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  srLabel.SetFont(WinUtil::systemFont, FALSE);

  onlyFree = BOOLSETTING(FREE_SLOTS_DEFAULT);

  ctrlPauseSearch.Create(m_hWnd, rcDefault, CTSTRING(PAUSE_SEARCH), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    BS_PUSHBUTTON, 0, IDC_SEARCH_PAUSE);
  ctrlPauseSearch.SetFont(WinUtil::systemFont);

  // Create listview columns
  WinUtil::splitTokens(columnIndexes, SETTING(SEARCHFRAME_ORDER), COLUMN_LAST);
  WinUtil::splitTokens(columnSizes, SETTING(SEARCHFRAME_WIDTHS), COLUMN_LAST);

  for(uint8_t j=0; j<COLUMN_LAST; j++) {
    int fmt = (j == COLUMN_SIZE || j == COLUMN_EXACT_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
    ctrlResults.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
  }

  ctrlResults.setColumnOrderArray(COLUMN_LAST, columnIndexes);
  ctrlResults.setAscending(false);
  ctrlResults.setVisible(SETTING(SEARCHFRAME_VISIBLE));
  ctrlResults.setSortColumn(COLUMN_HITS);

  ctrlResults.SetBkColor(WinUtil::bgColor);
  ctrlResults.SetTextBkColor(WinUtil::bgColor);
  ctrlResults.SetTextColor(WinUtil::textColor);
  ctrlResults.SetFont(WinUtil::systemFont, FALSE);	// use Util::font instead to obey Appearace settings
  ctrlResults.setFlickerFree(WinUtil::bgBrush);

  copyMenu.CreatePopupMenu();
  targetDirMenu.CreatePopupMenu();
  targetMenu.CreatePopupMenu();
  resultsMenu.CreatePopupMenu();

  copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_WMLINK, CTSTRING(COPY_MLINK_TEMPL)); // !SMT!-UI

  resultsMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
  resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS, CTSTRING(DOWNLOAD));
  resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetMenu, CTSTRING(DOWNLOAD_TO));
  resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, CTSTRING(DOWNLOAD_WHOLE_DIR));
  resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetDirMenu, CTSTRING(DOWNLOAD_WHOLE_DIR_TO));
  //resultsMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
  resultsMenu.AppendMenu(MF_SEPARATOR);
  //resultsMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
#ifdef PPA_INCLUDE_BITZI_LOOKUP
  resultsMenu.AppendMenu(MF_STRING, IDC_BITZI_LOOKUP, CTSTRING(BITZI_LOOKUP));
#endif
  resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
  resultsMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
  //resultsMenu.AppendMenu(MF_SEPARATOR);
  //appendUserItems(resultsMenu);
  resultsMenu.AppendMenu(MF_SEPARATOR);
  resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
  resultsMenu.SetMenuDefaultItem(IDC_DOWNLOAD_FAVORITE_DIRS);

  UpdateLayout();

  if(!initialString.empty()) {
    onEnter();
  }
  else {
    SetWindowText(CTSTRING(SEARCH));
    ::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), FALSE);
  }

  for (int j=0; j<COLUMN_LAST; j++) {
    ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
  }
  const ResourceManager::Strings* defaultColumnIndex = find(columnNames, columnNames + COUNTOF(columnNames), ResourceManager::USER);
  if (defaultColumnIndex != columnNames + COUNTOF(columnNames)) {
    ctrlFilterSel.SetCurSel(defaultColumnIndex - columnNames);
  }
  else {
    ctrlFilterSel.SetCurSel(0);
  }

  SettingsManager::getInstance()->addListener(this);
  m_headerHeight = ControlAdjuster::adjustHeaderHeight(m_hWnd);
  bHandled = FALSE;
  return 1;
}

void SearchFrame::onEnter() {
  StringList clients;	
  {
    ClientManager::LockInstance l_instance;
    Client::List& allClients = l_instance->getClients();
    for (Client::List::const_iterator it = allClients.begin(); it != allClients.end(); ++it) {
      const Client* client = *it;
      if (client->isConnected()) {
        clients.push_back(client->getHubUrl());
      }
    }
  }
  if (clients.empty()) {
    return;
  }

  tstring s = initialString;

  {
    Lock l(cs);
    search = StringTokenizer<tstring>(s, ' ').getTokens();
  }

  //strip out terms beginning with -
  tstring fullSearch = s;
  s.clear();
  for (TStringList::const_iterator si = search.begin(); si != search.end(); ++si)
    if ((*si)[0] != _T('-') || si->size() == 1) s += *si + _T(' ');	//Shouldn't get 0-length tokens, so safely assume at least a first char.
  s = s.substr(0, max(s.size(), static_cast<tstring::size_type>(1)) - 1);

  exactSize1 = (initialMode == SearchManager::SIZE_EXACT);
  exactSize2 = initialSize;		

  clearPausedResults();
  bPaused = false;
  ::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), TRUE);
  ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));

  // Start the countdown timer...
  // Can this be done in a better way?
  TimerManager::getInstance()->addListener(this);

  SearchManager::getInstance()->search(clients, 
    Text::fromT(s), 
    initialSize, 
    initialType, 
    initialMode, 
    "manual", 
    (int*) this, 
    fullSearch);
  searches++;
}

void SearchFrame::clearPausedResults() {
  for (SearchInfo::Iter i = PausedResults.begin(); i != PausedResults.end(); ++i) {
    delete *i;
  }
  PausedResults.clear();
}

void SearchFrame::on(SearchManagerListener::SR, SearchResult* aResult) throw() {
	// Check that this is really a relevant search result...
	{
		Lock l(cs);

		if(search.empty()) {
			return;
		}

		if(isHash) {
			if(aResult->getType() != SearchResult::TYPE_FILE || TTHValue(Text::fromT(search[0])) != aResult->getTTH()) {
				droppedResults++;
				PostMessage(WM_SPEAKER, FILTER_RESULT);
				return;
			}
		} else {
			// match all here
			for(TStringIter j = search.begin(); j != search.end(); ++j) {
				if((*j->begin() != _T('-') && Util::findSubString(aResult->getFile(), Text::fromT(*j)) == -1) ||
					(*j->begin() == _T('-') && j->size() != 1 && Util::findSubString(aResult->getFile(), Text::fromT(j->substr(1))) != -1)
					) 
				{
					droppedResults++;
					PostMessage(WM_SPEAKER, FILTER_RESULT);
					return;
				}
			}
		}
	}

/* 
#ifdef PPA_INCLUDE_IPFILTER
 		if (PGLoader::getInstance()->getIPBlockBool(aResult->getIP())) 
		{
 			droppedResults++;
 			PostMessage(WM_SPEAKER, FILTER_RESULT, NULL);
 			return;
 		}
#endif
*/

#ifdef PPA_INCLUDE_PG
	if(SETTING(PG_ENABLE) && SETTING(PG_SEARCH) && PGLoader::getInstance()->notAbused()) {
		// Reject these, not done in proper place but who cares
		// searching for a file isn't illegal anyway
 		string company = PGLoader::getInstance()->getIPBlock(aResult->getIP());
 		if (!company.empty()) {
 			droppedResults++;
 			PostMessage(WM_SPEAKER, FILTER_RESULT, NULL);
 			return;
 		}
 	}
#endif
	// Reject results without free slots
	if( (onlyFree && aResult->getFreeSlots() < 1) ||
	   (exactSize1 && (aResult->getSize() != exactSize2)))
	{
		droppedResults++;
		PostMessage(WM_SPEAKER, FILTER_RESULT);
		return;
	}

	SearchInfo* i = new SearchInfo(aResult);
	PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)i);
}

void SearchFrame::on(SearchManagerListener::Searching, SearchQueueItem* aSearch) throw() {
	if((searches >= 0) && (aSearch->getWindow() == (int*)this)) {
		searches--;
		dcassert(searches >= 0);
		PostMessage(WM_SPEAKER, SEARCH_START, (LPARAM)new SearchQueueItem(*aSearch));
	}
}

void SearchFrame::on(TimerManagerListener::Second, uint32_t aTick) throw() {
  if (searches > 0) {
    const uint32_t minSearchInterval = SETTING(MINIMUM_SEARCH_INTERVAL);
    const SearchManager* sm = SearchManager::getInstance();
	const uint32_t lastSearch = sm->getLastSearch();
	if (lastSearch == 0) {
		PostMessage(WM_SPEAKER, QUEUE_STATS, (LPARAM) -1);
	}
	else {
		const uint32_t waitFor = (lastSearch + minSearchInterval * 1000 - aTick) / 1000 + minSearchInterval * sm->getSearchQueueNumber((int*) this);
		PostMessage(WM_SPEAKER, QUEUE_STATS, (LPARAM) waitFor);
	}
  }
}

void SearchFrame::SearchInfo::view() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(Util::getTempPath() + sr->getFileName(),
				sr->getSize(), sr->getTTH(), sr->getUser(),
				QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt + si->columns[COLUMN_FILENAME]);
			QueueManager::getInstance()->add(target, si->sr->getSize(), 
				si->sr->getTTH(), si->sr->getUser());
			
			for(SearchInfo::Iter i = si->subItems.begin(); i != si->subItems.end(); ++i) {
				SearchInfo* j = *i;
				try {
					QueueManager::getInstance()->add(Text::fromT(tgt + si->columns[COLUMN_FILENAME]), j->sr->getSize(), j->sr->getTTH(), j->sr->getUser());
				} catch(const Exception&) {
				}
			}
			if(WinUtil::isShift())
				QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	try {
		QueueItem::Priority prio = WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT;
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->addDirectory(Text::fromT(si->columns[COLUMN_PATH]), 
			si->sr->getUser(), Text::fromT(tgt), prio);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), 
			Text::fromT(tgt), prio);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt);
			QueueManager::getInstance()->add(target, si->sr->getSize(), 
				si->sr->getTTH(), si->sr->getUser());

			if(WinUtil::isShift())
				QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), si->sr->getUser(), Text::fromT(tgt),
			WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::getList() {
	try {
		WinUtil::addInitalDir(sr->getUser(), Text::fromT(columns[COLUMN_PATH]));
		QueueManager::getInstance()->addList(sr->getUser(), QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::browseList() {
	try {
		QueueManager::getInstance()->addPfs(sr->getUser(), Text::fromT(columns[COLUMN_PATH]));
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::CheckTTH::operator()(SearchInfo* si) {
	if(firstTTH) {
		tth = si->columns[COLUMN_TTH];
		hasTTH = true;
		firstTTH = false;
	} else if(hasTTH) {
		if(tth != si->columns[COLUMN_TTH]) {
			hasTTH = false;
		}
	} 

	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID());
		firstHubs = false;
	} else if(!hubs.empty()) {
		// we will merge hubs of all users to ensure we can use OP commands in all hubs
		StringList sl = ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID());
		hubs.insert( hubs.end(), sl.begin(), sl.end() );
		//Util::intersect(hubs, ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID()));
	}
}

tstring SearchFrame::getDownloadDirectory(WORD wID) {
        TargetsMap::const_iterator i = dlTargets.find(wID);
        tstring dir = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
        if (i == dlTargets.end()) return dir;
        if (i->second == _T("?")) {
                if (WinUtil::browseDirectory(dir, m_hWnd)) {
                        WinUtil::addLastDir(dir);
                        return dir;
                } else {
                        return Util::emptyStringT;
                }
        }
        if (!i->second.empty()) dir = i->second;
        return dir;
}

LRESULT SearchFrame::onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		SearchInfo* si = ctrlResults.getItemData(i);
		SearchResult* sr = si->sr;
	
		if(sr->getType() == SearchResult::TYPE_FILE) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + si->columns[COLUMN_FILENAME];
			if(WinUtil::browseFile(target, m_hWnd)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(target));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(WinUtil::browseDirectory(target, m_hWnd)) {
				WinUtil::addLastDir(target);
				ctrlResults.forEachSelectedT(SearchInfo::Download(target));
			}
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			WinUtil::addLastDir(target);
			ctrlResults.forEachSelectedT(SearchInfo::Download(target));
		}
	}
	return 0;
}

LRESULT SearchFrame::onVideoDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		SearchInfo* si = ctrlResults.getItemData(i);
		SearchResult* sr = si->sr;
		if (sr->getType() == SearchResult::TYPE_FILE) {
			string localPath = ShareManager::getInstance()->toRealPath(sr->getTTH());
			if (!localPath.empty()) {
				WinUtil::showVideo(localPath);
			}
			else {
				QueueItem::List ql;
				QueueManager::getInstance()->getTargets(sr->getTTH(), ql);
				if (!ql.empty()) {
					ql.front()->setFlag(QueueItem::FLAG_ONLINE_VIDEO);
					WinUtil::showVideoSplash();
				}
				else {
					WinUtil::showVideoSplash();
					QueueManager::getInstance()->add(SETTING(DOWNLOAD_DIRECTORY) + sr->getFile(), sr->getSize(), sr->getTTH(), sr->getUser(), QueueItem::FLAG_ONLINE_VIDEO|QueueItem::FLAG_MULTI_SOURCE);
				}
			}
		}
	}
	return 0;
}

LRESULT SearchFrame::onDownload(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        tstring dir = getDownloadDirectory(wID);
        if (!dir.empty())
                ctrlResults.forEachSelectedT(SearchInfo::Download(dir));
        return 0;
}

LRESULT SearchFrame::onDownloadWhole(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        tstring dir = getDownloadDirectory(wID);
        if (!dir.empty())
                ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(dir));
        return 0;
}

LRESULT SearchFrame::onDownloadTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        tstring dir = getDownloadDirectory(wID);
        if (!dir.empty())
                ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(dir));
        return 0;
}

LRESULT SearchFrame::onDoubleClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlResults.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
	}
	return 0;
}

LRESULT SearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  SearchManager::getInstance()->stopSearch((int*)this);
  SettingsManager::getInstance()->removeListener(this);
  if(searches != 0) {
    searches--;
    TimerManager::getInstance()->removeListener(this);
  }
  SearchManager::getInstance()->removeListener(this);

  WinUtil::UnlinkStaticMenus(resultsMenu); // !SMT!-UI
  ctrlResults.SetRedraw(FALSE);
  ctrlResults.deleteAllItems();
  ctrlResults.SetRedraw(TRUE);

  clearPausedResults();

  ctrlResults.saveHeaderOrder(SettingsManager::SEARCHFRAME_ORDER, SettingsManager::SEARCHFRAME_WIDTHS, 
    SettingsManager::SEARCHFRAME_VISIBLE);

  bHandled = FALSE;
  return 0;
}

void SearchFrame::UpdateLayout(BOOL bResizeBars) {
  CRect rect;
  GetClientRect(&rect);
  UpdateBarsPosition(rect, bResizeBars);
  CRect rc(rect);
  rect.top += m_headerHeight;
  ctrlResults.MoveWindow(rect);
  rc.bottom = rc.top + m_headerHeight;
  if (ctrlFilterSel) {
    const SIZE sz = ControlAdjuster::adjustComboBoxSize(ctrlFilterSel);
    ctrlFilterSel.MoveWindow(rc.right - FILTER_RIGHT_MARGIN - sz.cx,
                             (m_headerHeight - ControlAdjuster::getComboBoxHeight(ctrlFilterSel)) / 2,
                             sz.cx,
                             sz.cy);
    rc.right -= sz.cx + FILTER_RIGHT_MARGIN;
  }
  if (ctrlFilter) {
    const SIZE sz = ControlAdjuster::adjustEditSize(ctrlFilter, FILTER_LENGTH);
    ctrlFilter.MoveWindow(rc.right - FILTER_RIGHT_MARGIN - sz.cx, 
                          (m_headerHeight - sz.cy) / 2,
                          sz.cx,
                          sz.cy);
    rc.right -= sz.cx + FILTER_RIGHT_MARGIN;
  }
  if (srLabel) {
    const SIZE sz = ControlAdjuster::adjustStaticSize(srLabel);
    srLabel.MoveWindow(rc.right - FILTER_RIGHT_MARGIN - sz.cx, 
                      (m_headerHeight - sz.cy) / 2,
                      sz.cx,
                      sz.cy);
    rc.right -= sz.cx + FILTER_RIGHT_MARGIN;
  }
  if (ctrlPauseSearch) {
    rc.left = rc.right - 100 - FILTER_RIGHT_MARGIN;
    const int buttonHeight = rc.Height() * 2 / 3;
    ctrlPauseSearch.MoveWindow(rc.left, (rc.top + rc.bottom - buttonHeight) / 2, 100, buttonHeight);
    rc.right = rc.left;
  }
  rc.left = rect.left;
  CClientDC dc(m_hWnd);
  m_header.updateLayout((HDC) dc, rc.Width(), rc.left, rc.top, rc.Height());
}

void SearchFrame::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		SearchResult* sr = ctrlResults.getItemData(sel)->sr;

		if(!sr->getUser()->isOnline())
			continue;

		if(uc.getType() == UserCommand::TYPE_RAW_ONCE) {
			if(users.find(sr->getUser()->getCID()) != users.end())
				continue;
			users.insert(sr->getUser()->getCID());
		}

		ucParams["fileFN"] = sr->getFile();
		ucParams["fileSI"] = Util::toString(sr->getSize());
		ucParams["fileSIshort"] = Util::formatBytes(sr->getSize());
		if(sr->getType() == SearchResult::TYPE_FILE) {
			ucParams["fileTR"] = sr->getTTH().toBase32();
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		StringMap tmp = ucParams;
		ClientManager::getInstance()->userCommand(sr->getUser(), uc, tmp, true);
	}
}

LRESULT SearchFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CDCHandle dc((HDC) wParam);
  dc.SetBkColor(WinUtil::bgColor);
  dc.SetTextColor(WinUtil::textColor);
  return (LRESULT) WinUtil::bgBrush;
}

LRESULT SearchFrame::onCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CDCHandle dc((HDC) wParam);
  dc.SetBkColor(GetSysColor(COLOR_3DFACE));
  dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
  return (LRESULT) GetSysColorBrush(COLOR_3DFACE);
}

LRESULT SearchFrame::onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        // !SMT!-UI
        int i = -1;
        while((i = ctrlResults.GetNextItem(i, LVNI_SELECTED)) != -1) {
		SearchResult* sr = ctrlResults.getItemData(i)->sr;
		if(sr->getType() == SearchResult::TYPE_FILE) {
			WinUtil::searchHash(sr->getTTH());
		}
	}
	return 0;
}

#ifdef PPA_INCLUDE_BITZI_LOOKUP

 LRESULT SearchFrame::onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
 	if(ctrlResults.GetSelectedCount() == 1) {
 		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
 		SearchResult* sr = ctrlResults.getItemData(i)->sr;
 		if(sr->getType() == SearchResult::TYPE_FILE) {
 			WinUtil::bitziLink(sr->getTTH());
 		}
 	}
 	return 0;
 }
#endif

LRESULT SearchFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
 	switch(wParam) {
	case ADD_RESULT:
		{
			SearchInfo* si = (SearchInfo*)lParam;
			SearchResult* sr = si->sr;
            // Check previous search results for dupes
			if(!si->columns[COLUMN_TTH].empty() && useGrouping) {
				SearchInfo* main = ctrlResults.findMainItem(si->columns[COLUMN_TTH]);
				if(main) {
					if((sr->getUser()->getCID() == main->sr->getUser()->getCID()) && (sr->getFile() == main->sr->getFile())) {	 	
						delete si;	 	
						return 0;	 	
					} 	
					for(SearchInfo::Iter k = main->subItems.begin(); k != main->subItems.end(); ++k){	 	
						if((sr->getUser()->getCID() == (*k)->getUser()->getCID()) && (sr->getFile() == (*k)->sr->getFile())) {	 	
							delete si;	 	
							return 0;	 	
						} 	
					}	 	
				}
			} else {
				for(vector<SearchInfo*>::const_iterator s = ctrlResults.mainItems.begin(); s != ctrlResults.mainItems.end(); ++s) {
					SearchInfo* si2 = *s;
	                SearchResult* sr2 = si2->sr;
					if((sr->getUser()->getCID() == sr2->getUser()->getCID()) && (sr->getFile() == sr2->getFile())) {
						delete si;	 	
				        return 0;	 	
					}
				}	 	
                        }
			if(!bPaused) {
				++resultsCount;
				if(!si->columns[COLUMN_TTH].empty() && useGrouping) {
					ctrlResults.insertGroupedItem(si, expandSR);
				} else {
					ctrlResults.insertItem(si, si->imageIndex());
					ctrlResults.mainItems.push_back(si);
				}

				if(!filter.empty())
					updateSearchList(si);

				if (BOOLSETTING(BOLD_SEARCH)) {
					setDirty();
				}
                                updateSearchResultCount();

                                if (resultsCount % 13 == 0) {
                                  ctrlResults.resort();
                                }
			} else {
				PausedResults.push_back(si);
				updateSearchResultCount();
			}
		}
		break;
	case FILTER_RESULT:
		// TODO ctrlStatus.SetText(3, (Util::toStringW(droppedResults) + _T(" ") + TSTRING(FILTERED)).c_str());
		break;
	case QUEUE_STATS:
		{
			TCHAR buf[256];
			if (lParam == -1) {
				_tcscpy(buf, CTSTRING(WAITING_FOR_CONNECTION));
			}
			else {
				_stprintf(buf, CTSTRING(WAITING_FOR), lParam);
			}
			if (m_header.size() > m_initialWordCount) {
				m_header.removeWords(m_initialWordCount, m_header.size());
				m_wordSearchText.clear();
				m_wordFileCount.clear();
			}
			m_header.addWord(_T("-"), m_header.textFont, GetSysColor(COLOR_WINDOWTEXT));
			m_header.addWords(buf, m_header.textFont, GetSysColor(COLOR_WINDOWTEXT));
			m_header.updateLayout();
			SetWindowText(buf);
		}
		break;
	case SEARCH_START:
		{
			SearchQueueItem* aSearch = (SearchQueueItem*)(lParam);

			ctrlResults.deleteAllItems();
			resultsCount = 0;

			{
				Lock l(cs);
				search = StringTokenizer<tstring>(aSearch->getSearch(), ' ').getTokens();
			}
			isHash = (aSearch->getTypeMode() == SearchManager::TYPE_TTH);

			// Turn off the countdown timer if no more manual searches left
			if(searches == 0)
				TimerManager::getInstance()->removeListener(this);

			const tstring searchText = Text::toT(aSearch->getTarget());
            updateSearchText(searchText);
	
			SetWindowText((TSTRING(SEARCH) + _T(" - ") + searchText).c_str());
			delete aSearch;

			droppedResults = 0;
		}
		break;
	}

	return 0;
}

void SearchFrame::updateSearchText(const tstring& searchText) {
	m_header.removeWords(m_initialWordCount, m_header.size());
	m_wordFileCount.clear();
	m_header.addWords(_T("(по запросу"), m_header.textFont, GetSysColor(COLOR_WINDOWTEXT));
	m_wordSearchText = m_header.addWord(searchText, m_header.textFont, GREEN_COLOR);
	m_header.addWords(_T(")"), m_header.textFont, GetSysColor(COLOR_WINDOWTEXT));
	m_header.updateLayout();
}

void SearchFrame::updateSearchResultCount() {
	const tstring countText = PausedResults.empty() ? Util::toStringW(resultsCount) : Util::toStringW(resultsCount) + _T(" / ") + Util::toStringW(resultsCount + PausedResults.size());
	if (m_wordFileCount) {
		m_header.setText(m_wordFileCount, countText);
	}
	else {
		if (!m_wordSearchText) {
			updateSearchText(_T("-"));
		}
		const int index = m_header.indexOf(m_wordSearchText);
		dcassert(index >= 0);
		m_header.removeWords(index + 1, m_header.size());
		m_header.addWords(_T("- найдено"), m_header.textFont, GetSysColor(COLOR_WINDOWTEXT));
		m_wordFileCount = m_header.addWord(countText, m_header.textFont, GREEN_COLOR);
		m_header.addWords(_T("файлов)"), m_header.textFont, GetSysColor(COLOR_WINDOWTEXT));
	}
}

LRESULT SearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlResults && ctrlResults.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlResults, pt);
		}
		
		if(ctrlResults.GetSelectedCount() > 0) {

                        updateSummary(); // !SMT!-UI
                        dlTargets.clear(); // !SMT!-S

                        SearchInfo* si = NULL;
                        SearchResult* sr = NULL;
                        if(ctrlResults.GetSelectedCount() == 1) {
                                int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
                                dcassert(i != -1);
                                si = ctrlResults.getItemData(i);
                                sr = si->sr;
                        }

                        // first sub-menu
                        while(targetMenu.GetMenuItemCount() > 0) {
                                targetMenu.DeleteMenu(0, MF_BYPOSITION);
                        }

                        dlTargets[IDC_DOWNLOAD_FAVORITE_DIRS + 0] = Util::emptyStringT; // for 'Download' without options

                        targetMenu.InsertSeparatorFirst(TSTRING(DOWNLOAD_TO));
                        //Append favorite download dirs
                        StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
                        int n = 1;
                        if (spl.size() > 0) {
                                for(StringPairIter i = spl.begin(); i != spl.end(); i++) {
                                        const tstring tar = Text::toT(i->second); // !SMT!-S
                                        dlTargets[IDC_DOWNLOAD_FAVORITE_DIRS + n] = Text::toT(i->first);
                                        targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, tar.c_str());
                                        n++;
                                }
                                targetMenu.AppendMenu(MF_SEPARATOR);
                        }

                        // !SMT!-S: Append special folder, like in source share
                        if (si) {
                                tstring srcpath = si->columns[COLUMN_PATH];
                                if (srcpath.length() > 2) {
                                        unsigned start = srcpath.substr(0,srcpath.length()-1).find_last_of(_T('\\'));
                                        if (start == srcpath.npos) start = 0;
                                        srcpath = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + srcpath.substr(start+1);
                                        dlTargets[IDC_DOWNLOAD_FAVORITE_DIRS + n] = srcpath;
                                        targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, srcpath.c_str());
                                        n++;
                                }
                        }

                        targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
                        n++;

                        //Append last favorite download dirs
                        if(WinUtil::lastDirs.size() > 0) {
                                targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
                                for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
                                        const tstring& tar = *i; // !SMT!-S
                                        dlTargets[IDC_DOWNLOAD_FAVORITE_DIRS + n] = tar;
                                        targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, tar.c_str());
                                        n++;
                                }
                        }

			SearchInfo::CheckTTH cs = ctrlResults.forEachSelectedT(SearchInfo::CheckTTH());

			if(cs.hasTTH) {
				targets.clear();
				QueueManager::getInstance()->getTargets(TTHValue(Text::fromT(cs.tth)), targets);

                                if(targets.size() > 0) {
                                        targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
                                        for(StringIter i = targets.begin(); i != targets.end(); ++i) {
                                                const tstring& tar = Text::toT(*i); // !SMT!-S
                                                dlTargets[IDC_DOWNLOAD_TARGET + n] = tar;
                                                targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, tar.c_str());
                                                n++;
                                        }
                                }
                        }

                        // second sub-menu
                        while(targetDirMenu.GetMenuItemCount() > 0) {
                                targetDirMenu.DeleteMenu(0, MF_BYPOSITION);
                        }

                        dlTargets[IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + 0] = Util::emptyStringT; // for 'Download whole dir' without options
                        targetDirMenu.InsertSeparatorFirst(TSTRING(DOWNLOAD_WHOLE_DIR_TO));
                        //Append favorite download dirs
                        n = 1;
                        if (spl.size() > 0) {
                                for(StringPairIter i = spl.begin(); i != spl.end(); ++i) {
                                        const tstring tar = Text::toT(i->second); // !SMT!-S
                                        dlTargets[IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n] = Text::toT(i->first);
                                        targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n, tar.c_str());
                                        n++;
                                }
                                targetDirMenu.AppendMenu(MF_SEPARATOR);
                        }

                        dlTargets[IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n] = _T("?");
                        targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n, CTSTRING(BROWSE));
                        n++;

                        if(WinUtil::lastDirs.size() > 0) {
                                targetDirMenu.AppendMenu(MF_SEPARATOR);
                                for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
                                        const tstring tar = *i; // !SMT!-S
                                        dlTargets[IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n] = tar;
                                        targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n, i->c_str());
                                        n++;
                                }
                        }
						if (sr != NULL && WinUtil::isVideo(sr->getFile())) {
							if (resultsMenu.GetMenuItemID(2) != IDC_VIDEO) {
								resultsMenu.InsertMenu(2, MF_BYPOSITION | MF_STRING, IDC_VIDEO, CTSTRING(FILELIST_FILEMENU_VIDEO));
							}
						}
						else {
							resultsMenu.RemoveMenu(IDC_VIDEO, MF_BYCOMMAND);
						}

                        prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, cs.hubs);
                        checkAdcItems(resultsMenu);

                        copyMenu.InsertSeparatorFirst(TSTRING(USERINFO));
                        resultsMenu.InsertSeparatorFirst(sr? Text::toT(sr->getFileName()) : CTSTRING(FILES));
			resultsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
                        resultsMenu.RemoveFirstItem();
                        copyMenu.RemoveFirstItem();

                        cleanMenu(resultsMenu);
                        return TRUE;
                }
        }
        bHandled = FALSE;
        return FALSE;
}

LRESULT SearchFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlResults.forEachSelected(&SearchInfo::getList);
	return 0;
}

LRESULT SearchFrame::onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlResults.forEachSelected(&SearchInfo::browseList);
	return 0;
}

void SearchFrame::SearchInfo::update() { 
	if(sr->getType() == SearchResult::TYPE_FILE) {
		if(sr->getFile().rfind(_T('\\')) == tstring::npos) {
			columns[COLUMN_FILENAME] = Text::toT(sr->getFile());
		} else {
			columns[COLUMN_FILENAME] = Text::toT(Util::getFileName(sr->getFile()));
			columns[COLUMN_PATH] = Text::toT(Util::getFilePath(sr->getFile()));
		}

		columns[COLUMN_TYPE] = Text::toT(Util::getFileExt(Text::fromT(columns[COLUMN_FILENAME])));
		if(!columns[COLUMN_TYPE].empty() && columns[COLUMN_TYPE][0] == _T('.'))
			columns[COLUMN_TYPE].erase(0, 1);
		columns[COLUMN_SIZE] = Util::formatBytesW(sr->getSize());
		columns[COLUMN_EXACT_SIZE] = Util::formatExactSize(sr->getSize());
	} else {
		columns[COLUMN_FILENAME] = Text::toT(sr->getFileName());
		columns[COLUMN_PATH] = Text::toT(sr->getFile());
		columns[COLUMN_TYPE] = TSTRING(DIRECTORY);
		if(sr->getSize() > 0) {
			columns[COLUMN_SIZE] = Util::formatBytesW(sr->getSize());
			columns[COLUMN_EXACT_SIZE] = Util::formatExactSize(sr->getSize());
		}
	}
	columns[COLUMN_NICK] = Text::toT(sr->getUser()->getFirstNick());
//[-]PPA	columns[COLUMN_CONNECTION] = Text::toT(ClientManager::getInstance()->getConnection(sr->getUser()->getCID()));
	columns[COLUMN_HUB] = Text::toT(sr->getHubName());
	columns[COLUMN_SLOTS] = Text::toT(sr->getSlotString());
    columns[COLUMN_IP] = Text::toT(sr->getIP());
#ifdef PPA_INCLUDE_DNS
    columns[COLUMN_DNS] = Text::toT(Socket::nslookup(sr->getIP()));
#endif
	flagimage = 0;
	if (!columns[COLUMN_IP].empty()) 
	{
		// Only attempt to grab a country mapping if we actually have an IP address
                string aCountry = Util::getIpCountry(sr->getIP());
                if(!aCountry.empty()) {
                        flagimage = WinUtil::getFlagImage(aCountry);
                        columns[COLUMN_LOCATION] = Text::toT(aCountry);
                }
        }
	if(sr->getType() == SearchResult::TYPE_FILE) {
		columns[COLUMN_TTH] = Text::toT(sr->getTTH().toBase32());
		columns[COLUMN_LOCAL_PATH] = Text::toT(ShareManager::getInstance()->toRealPath(sr->getTTH()));
	}
}

LRESULT SearchFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        string data;
        // !SMT!-UI: copy several rows
        int i = -1;
        while( (i = ctrlResults.GetNextItem(i, LVNI_SELECTED)) != -1) {
                SearchResult* sr = ctrlResults.getItemData(i)->sr;
	string sCopy;
		switch (wID) {
			case IDC_COPY_NICK:
				sCopy = sr->getUser()->getFirstNick();
				break;
			case IDC_COPY_FILENAME:
				sCopy = Util::getFileName(sr->getFile());
				break;
			case IDC_COPY_PATH:
				sCopy = Util::getFilePath(sr->getFile());
				break;
			case IDC_COPY_SIZE:
				sCopy = Util::formatBytes(sr->getSize());
				break;
			case IDC_COPY_LINK:
                                if(sr->getType() == SearchResult::TYPE_FILE)
                                        sCopy = WinUtil::getMagnet(sr->getTTH(), Text::toT(sr->getFileName()), sr->getSize());
				break;
                        case IDC_COPY_WMLINK: // !SMT!-UI
                                if(sr->getType() == SearchResult::TYPE_FILE)
                                        sCopy = WinUtil::getWebMagnet(sr->getTTH(), Text::toT(sr->getFileName()), sr->getSize());
                                break;
			case IDC_COPY_TTH:
				if(sr->getType() == SearchResult::TYPE_FILE)
					sCopy = sr->getTTH().toBase32();
				break;
			default:
				dcdebug("SEARCHFRAME DON'T GO HERE\n");
				return 0;
		}
                if (!sCopy.empty()) {
                        if (data.empty())
                                data = sCopy;
                        else
                                data = data + "\r\n" + sCopy;
	}
        }
        if (!data.empty())
                WinUtil::setClipboard(Text::toT(data));
        return 0;
}

LRESULT SearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		cd->clrText = WinUtil::textColor;	
		SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
		if(si->sr != NULL) {
			targets.clear();
			QueueManager::getInstance()->getTargets(TTHValue(si->sr->getTTH().toBase32()), targets);
			if(si->sr->getType() == SearchResult::TYPE_FILE && targets.size() > 0) {		
				cd->clrText = SETTING(SEARCH_ALTERNATE_COLOUR);	
			}
                        if (ShareManager::getInstance()->isTTHShared(si->sr->getTTH())) // !SMT!-S
                                cd->clrTextBk = SETTING(DUPE_COLOR); // !SMT!-S
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
		{
		if(/*[-]PPA BOOLSETTING(GET_USER_COUNTRY) && */
                 (ctrlResults.findColumn(cd->iSubItem) == COLUMN_LOCATION)) 
		{
			SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
			ctrlResults.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			COLORREF color;
			if(ctrlResults.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
				if(ctrlResults.m_hWnd == ::GetFocus()) {
					color = GetSysColor(COLOR_HIGHLIGHT);
					SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				} else {
					color = GetBkColor(cd->nmcd.hdc);
					SetBkColor(cd->nmcd.hdc, color);
				}				
			} else {
				color = WinUtil::bgColor;
				SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
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
			ctrlResults.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				LONG top = rc2.top + (rc2.Height() - 15)/2;
				if((top - rc2.top) < 2)
					top = rc2.top + 1;

				POINT p = { rc2.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, si->getFlagImage(), p, LVSIL_SMALL);
				top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		}		
	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT SearchFrame::onFilterChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  filter = ctrlFilter.getText();
  ctrlFilter.setClearButtonVisible(!filter.empty());
  updateSearchList();
  return 0;
}

bool SearchFrame::parseFilter(FilterModes& mode, int64_t& size) {
	tstring::size_type start = (tstring::size_type)tstring::npos;
	tstring::size_type end = (tstring::size_type)tstring::npos;
	int64_t multiplier = 1;
	
	if(filter.compare(0, 2, _T(">=")) == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("<=")) == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("==")) == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("!=")) == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filter[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filter[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filter[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == tstring::npos)
		return false;
	if(filter.length() <= start)
		return false;

	if((end = Util::findSubString(filter, _T("TiB"))) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filter, _T("GiB"))) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filter, _T("MiB"))) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filter, _T("KiB"))) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filter, _T("TB"))) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filter, _T("GB"))) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filter, _T("MB"))) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filter, _T("kB"))) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filter, _T("B"))) != tstring::npos) {
		multiplier = 1;
	}


	if(end == tstring::npos) {
		end = filter.length();
	}
	
	tstring tmpSize = filter.substr(start, end-start);
	size = static_cast<int64_t>(Util::toDouble(Text::fromT(tmpSize)) * multiplier);
	
	return true;
}

bool SearchFrame::matchFilter(SearchInfo* si, int sel, bool doSizeCompare, FilterModes mode, int64_t size, PME* reg) {
        if(filter.empty())
                return true;

        if(doSizeCompare) {
                bool insert = true;
		switch(mode) {
			case EQUAL: insert = (size == si->sr->getSize()); break;
			case GREATER_EQUAL: insert = (size <=  si->sr->getSize()); break;
			case LESS_EQUAL: insert = (size >=  si->sr->getSize()); break;
			case GREATER: insert = (size < si->sr->getSize()); break;
			case LESS: insert = (size > si->sr->getSize()); break;
			case NOT_EQUAL: insert = (size != si->sr->getSize()); break;
		}
	return insert;
        } else if (reg) {
                return reg->match(Text::toLower(si->getText(sel))) > 0; // !SMT!-fix
        }
        return true;
}
	

void SearchFrame::updateSearchList(SearchInfo* si) {
	int64_t size = -1;
	FilterModes mode = NONE;

	int sel = ctrlFilterSel.GetCurSel();
	bool doSizeCompare = sel == COLUMN_SIZE && parseFilter(mode, size);

        PME reg(Text::toLower(filter), _T("")); // !SMT!-fix
        PME* aReg = NULL;

        if(reg.IsValid() && !filter.empty())
                aReg = &reg;

	if(si != NULL) {
                if(!matchFilter(si, sel, doSizeCompare, mode, size, aReg))
			ctrlResults.deleteItem(si);
	} else {
		ctrlResults.SetRedraw(FALSE);
		ctrlResults.DeleteAllItems();

		for(vector<SearchInfo*>::const_iterator i = ctrlResults.mainItems.begin(); i != ctrlResults.mainItems.end(); ++i) {
			SearchInfo* si = *i;
			si->collapsed = true;
                        if(matchFilter(si, sel, doSizeCompare, mode, size, aReg)) {
				dcassert(ctrlResults.findItem(si) == -1);
				int k = ctrlResults.insertItem(si, si->imageIndex());
				if(si->subItems.size() > 0) {
					if(si->collapsed) {
						ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);	
					} else {
						ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);	
					}
				} else {
					ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);	
				}
			}
		}
		ctrlResults.SetRedraw(TRUE);
	}
}

LRESULT SearchFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (!filter.empty()) {
	updateSearchList();
  }
	return 0;
}

void SearchFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlResults.GetBkColor() != WinUtil::bgColor) {
		ctrlResults.SetBkColor(WinUtil::bgColor);
		ctrlResults.SetTextBkColor(WinUtil::bgColor);
		ctrlResults.setFlickerFree(WinUtil::bgBrush);
		refresh = true;
	}
	if(ctrlResults.GetTextColor() != WinUtil::textColor) {
		ctrlResults.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT SearchFrame::onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (bPaused) {
    bPaused = false;
    for (SearchInfo::Iter i = PausedResults.begin(); i != PausedResults.end(); ++i) {
      PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)(*i));
    }
    PausedResults.clear();
    //ctrlStatus.SetText(2, (Util::toStringW(ctrlResults.GetItemCount()) + _T(" ") + TSTRING(FILES)).c_str());			
    ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
  }
  else {
    bPaused = true;
    ctrlPauseSearch.SetWindowText(CTSTRING(CONTINUE_SEARCH));
  }
  return 0;
}

void SearchFrame::onActivate() {
  if (++windowActivationCount > 1) {
    if (BOOLSETTING(SEARCH_EXPAND_TOOLBAR_ON_ACTIVATE)) {
      PeersSearchBar::getInstance()->expand();
    }
    if (BOOLSETTING(SEARCH_RESTORE_CONDITION_ON_ACTIVATE)) {
      PeersSearchBar* tb = PeersSearchBar::getInstance();
      tb->setSearchText(initialString);
      tb->setSearchFileType(initialType);
      tb->setSearchSize(initialMode, initialSize);
    }
  }
}

/**
 * @file
 * $Id: SearchFrm.cpp,v 1.37.2.1 2008/12/17 19:28:04 alexey Exp $
 */
