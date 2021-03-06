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
#include "SpyFrame.h"
#include "../peers/SearchFrmFactory.h"
#include "../client/ShareManager.h"
#include "../client/ConnectionManager.h"

int SpyFrame::columnSizes[] = { 305, 70, 90, 85, 20 };
int SpyFrame::columnIndexes[] = { COLUMN_STRING, COLUMN_COUNT, COLUMN_USERS, COLUMN_TIME, COLUMN_SHARE_HIT }; // !SMT!-S
static ResourceManager::Strings columnNames[] = { ResourceManager::SEARCH_STRING, ResourceManager::COUNT, ResourceManager::USERS, ResourceManager::TIME, ResourceManager::SHARED }; // !SMT!-S

LRESULT SpyFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlSearches.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, WS_EX_CLIENTEDGE, IDC_RESULTS);
	ctrlSearches.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	ctrlSearches.SetBkColor(WinUtil::bgColor);
	ctrlSearches.SetTextBkColor(WinUtil::bgColor);
	ctrlSearches.SetTextColor(WinUtil::textColor);

	ctrlIgnoreTth.Create(ctrlStatus.m_hWnd, rcDefault, CTSTRING(IGNORE_TTH_SEARCHES), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlIgnoreTth.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlIgnoreTth.SetFont(WinUtil::systemFont);
	ctrlIgnoreTth.SetCheck(ignoreTth);
	ignoreTthContainer.SubclassWindow(ctrlIgnoreTth.m_hWnd);

	WinUtil::splitTokens(columnIndexes, SETTING(SPYFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SPYFRAME_WIDTHS), COLUMN_LAST);
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_COUNT) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlSearches.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlSearches.setSort(COLUMN_COUNT, ExListViewCtrl::SORT_INT, false);
        //ctrlSearches.setVisible(SETTING(SPYFRAME_VISIBLE)); // !SMT!-UI

	ShareManager::getInstance()->setHits(0);

	bHandled = FALSE;
	return 1;
}

LRESULT SpyFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed){
		ClientManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		bHandled = TRUE;
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlSearches, SettingsManager::SPYFRAME_ORDER, SettingsManager::SPYFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
                //ctrlSearches.saveHeaderOrder(SettingsManager::SPYFRAME_ORDER, SettingsManager::SPYFRAME_WIDTHS, SettingsManager::SPYFRAME_VISIBLE); // !SMT!-UI
		if (ignoreTth != BOOLSETTING(SPY_FRAME_IGNORE_TTH_SEARCHES))
			SettingsManager::getInstance()->set(SettingsManager::SPY_FRAME_IGNORE_TTH_SEARCHES, ignoreTth);

		WinUtil::setButtonPressed(IDC_SEARCH_SPY, false);
		bHandled = FALSE;
		return 0;
	}
}

LRESULT SpyFrame::onColumnClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlSearches.getSortColumn()) {
		if (!ctrlSearches.isAscending())
			ctrlSearches.setSort(-1, ctrlSearches.getSortType());
		else
			ctrlSearches.setSortDirection(false);
	} else {
		if(l->iSubItem == COLUMN_COUNT) {
			ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
		} else {
			ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
		}
	}
	return 0;
}

void SpyFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);

		int tmp = (sr.Width()) > 616 ? 516 : ((sr.Width() > 116) ? sr.Width()-100 : 16);

		w[0] = 150;
		w[1] = sr.right - tmp - 150;
		w[2] = w[1] + (tmp-16)*1/4;
		w[3] = w[1] + (tmp-16)*2/4;
		w[4] = w[1] + (tmp-16)*3/4;
		w[5] = w[1] + (tmp-16)*4/4;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
		ctrlIgnoreTth.MoveWindow(sr);
	}

	ctrlSearches.MoveWindow(&rect);
}

LRESULT SpyFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == SEARCH) {
                SearchInfo* x = (SearchInfo*)lParam;

                SearchIter it2 = searches.find(x->s);
                if(it2 == searches.end()) {
                        searches[x->s].i = 1;
                } 

                if (::strncmp(x->seeker.c_str(), "Hub:", 4) && x->seeker.find(':') != string::npos) {
                        x->seeker = x->seeker.substr(0, x->seeker.find(':'));
                        UserPtr u = ClientManager::getInstance()->getUserByIp(x->seeker);
                        if (u) x->seeker = u->getFirstNick();
                }

                int k;
                for (k = 0; k < NUM_SEEKERS; ++k)
                        if (x->seeker == (searches[x->s].seekers)[k])
                                break;          //that user's searching for file already noted

                {
                        Lock l(cs);
                        if (k == NUM_SEEKERS)           //loop terminated normally
                                searches[x->s].AddSeeker(x->seeker);
                }

                string temp;

                for (int k = 0; k < NUM_SEEKERS; ++k)
                        temp += (searches[x->s].seekers)[k] + "  ";

		total++;

		// Not thread safe, but who cares really...
		perSecond[cur]++;

                // !SMT!-S
                string hit;
                if (x->re == ClientManagerListener::SEARCH_PARTIAL_HIT) hit = "*";
                if (x->re == ClientManagerListener::SEARCH_HIT) hit = "+";

                int j = ctrlSearches.find(Text::toT(x->s));
		if(j == -1) {
			TStringList a;
                        a.push_back(Text::toT(x->s));
                        a.push_back(Text::toT(Util::toString(1)));
                        a.push_back(Text::toT(temp));
			a.push_back(Text::toT(Util::getTimeString()));			
                        a.push_back(Text::toT(hit));
                        ctrlSearches.insert(a, 0, x->re);// !SMT!-S
			if(ctrlSearches.GetItemCount() > 500) {
				ctrlSearches.DeleteItem(ctrlSearches.GetItemCount() - 1);
			}
		} else {
			TCHAR tmp[32];
			ctrlSearches.GetItemText(j, COLUMN_COUNT, tmp, 32);
			ctrlSearches.SetItemText(j, COLUMN_COUNT, Util::toStringW(Util::toInt(Text::fromT(tmp))+1).c_str());
			ctrlSearches.SetItemText(j, COLUMN_USERS, Text::toT(temp).c_str());
			ctrlSearches.GetItemText(j, COLUMN_TIME, tmp, 32);
			ctrlSearches.SetItemText(j, COLUMN_TIME, Text::toT(Util::getTimeString()).c_str());
                        ctrlSearches.SetItemText(j, COLUMN_SHARE_HIT, Text::toT(hit.c_str()).c_str()); // !SMT!-S
                        ctrlSearches.SetItem(j, COLUMN_SHARE_HIT, LVIF_PARAM, NULL, 0, 0, 0, x->re); // !SMT!-S
			if(ctrlSearches.getSortColumn() == COLUMN_COUNT )
				ctrlSearches.resort();
			if(ctrlSearches.getSortColumn() == COLUMN_TIME )
				ctrlSearches.resort();
		}
		delete x;

		ctrlStatus.SetText(2, (TSTRING(TOTAL) + Util::toStringW(total)).c_str());
		ctrlStatus.SetText(4, (TSTRING(HITS) + Util::toStringW(ShareManager::getInstance()->getHits())).c_str());
		double ratio = total > 0 ? ((double)ShareManager::getInstance()->getHits()) / (double)total : 0.0;
		ctrlStatus.SetText(5, (TSTRING(HIT_RATIO) + Util::toStringW(ratio)).c_str());
	} else if(wParam == TICK_AVG) {
		float* x = (float*)lParam;
		ctrlStatus.SetText(3, (TSTRING(AVERAGE) + Util::toStringW(*x)).c_str());
		delete x;
	}

	return 0;
}

LRESULT SpyFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlSearches && ctrlSearches.GetSelectedCount() == 1) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlSearches, pt);
		}

		int i = ctrlSearches.GetNextItem(-1, LVNI_SELECTED);

		CMenu mnu;
		mnu.CreatePopupMenu();
		mnu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));
		TCHAR buf[256];
		ctrlSearches.GetItemText(i, COLUMN_STRING, buf, 256);
		searchString = buf;

		mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		
		return TRUE; 
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT SpyFrame::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (Util::strnicmp(searchString.c_str(), _T("TTH:"), 4) == 0) {
    SearchFrameFactory::searchTTH(searchString.substr(4));
  }
  else {
    SearchFrameFactory::openWindow(searchString);
  }
  return 0;
}

void SpyFrame::on(ClientManagerListener::IncomingSearch, const string& user, const string& s, ClientManagerListener::SearchReply re) throw() {
	if(ignoreTth && s.compare(0, 4, "TTH:") == 0)
		return;
        SearchInfo *x = new SearchInfo(user, s, re);
        string::size_type i = 0;
        while( (i=(x->s).find('$')) != string::npos) {
                (x->s)[i] = ' ';
	}
	PostMessage(WM_SPEAKER, SEARCH, (LPARAM)x);
}

void SpyFrame::on(TimerManagerListener::Second, uint32_t) throw() {
	float* f = new float(0.0);
	for(int i = 0; i < AVG_TIME; ++i) {
		(*f) += (float)perSecond[i];
	}
	(*f) /= AVG_TIME;
		
	cur = (cur + 1) % AVG_TIME;
	perSecond[cur] = 0;
	PostMessage(WM_SPEAKER, TICK_AVG, (LPARAM)f);
}

void SpyFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlSearches.GetBkColor() != WinUtil::bgColor) {
		ctrlSearches.SetBkColor(WinUtil::bgColor);
		ctrlSearches.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlSearches.GetTextColor() != WinUtil::textColor) {
		ctrlSearches.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

// !SMT!-S
LRESULT SpyFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
        NMLVCUSTOMDRAW* plvcd = reinterpret_cast<NMLVCUSTOMDRAW*>( pnmh );

        if( CDDS_PREPAINT == plvcd->nmcd.dwDrawStage )
                return CDRF_NOTIFYITEMDRAW;

        if( CDDS_ITEMPREPAINT == plvcd->nmcd.dwDrawStage ) {
                ClientManagerListener::SearchReply re = (ClientManagerListener::SearchReply)(plvcd->nmcd.lItemlParam);

                //check if the file or dir is a dupe, then use the dupesetting color
                if (re == ClientManagerListener::SEARCH_HIT)
                        plvcd->clrTextBk = SETTING(DUPE_COLOR);
                else if(re == ClientManagerListener::SEARCH_PARTIAL_HIT)
                        //if it's a partial hit, try to use some simple blending
                        plvcd->clrTextBk = WinUtil::blendColors(SETTING(DUPE_COLOR), SETTING(BACKGROUND_COLOR));
        }
        return CDRF_DODEFAULT;
}

/**
 * @file
 * $Id: SpyFrame.cpp,v 1.3 2008/03/10 06:57:19 alexey Exp $
 */
