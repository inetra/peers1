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

#if !defined(SPY_FRAME_H)
#define SPY_FRAME_H


#include "../client/ClientManager.h"
#include "../client/TimerManager.h"

#include "ExListViewCtrl.h"

#define IGNORETTH_MESSAGE_MAP 7

class SpyFrame : 
  public MDITabChildWindowImpl<SpyFrame, IDR_SPY>, 
  public StaticFrame<SpyFrame, ResourceManager::SEARCH_SPY, IDC_SEARCH_SPY>,
  private ClientManagerListener,
  private TimerManagerListener,
  private SettingsManagerListener
{
public:
	SpyFrame() : total(0), cur(0), closed(false), ignoreTth(BOOLSETTING(SPY_FRAME_IGNORE_TTH_SEARCHES)), ignoreTthContainer(WC_BUTTON, this, IGNORETTH_MESSAGE_MAP) {
		memzero(perSecond, sizeof(perSecond));
		ClientManager::getInstance()->addListener(this);
		TimerManager::getInstance()->addListener(this);
		SettingsManager::getInstance()->addListener(this);
	}

	~SpyFrame() { }

	enum {
		COLUMN_FIRST,
		COLUMN_STRING = COLUMN_FIRST,
		COLUMN_COUNT,
                COLUMN_USERS,
		COLUMN_TIME,
                COLUMN_SHARE_HIT, // !SMT!-S
		COLUMN_LAST
	};

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	DECLARE_FRAME_WND_CLASS_EX(_T("SpyFrame"), IDR_SPY, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SpyFrame, IDR_SPY> baseClass;
	BEGIN_MSG_MAP(SpyFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, onColumnClickResults)
                NOTIFY_HANDLER(IDC_RESULTS, NM_CUSTOMDRAW, onCustomDraw) // !SMT!-S
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(IGNORETTH_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onIgnoreTth)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onColumnClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
        LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/); // !SMT!-S

	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onIgnoreTth(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		ignoreTth = (wParam == BST_CHECKED);
		return 0;
	}
	
private:

	enum { AVG_TIME = 60 };
	enum {
		SEARCH,
		TICK_AVG
	};

	ExListViewCtrl ctrlSearches;
	CStatusBarCtrl ctrlStatus;
	CContainedWindow ignoreTthContainer;
	CButton ctrlIgnoreTth;
	int total;
	int perSecond[AVG_TIME];
	int cur;
	tstring searchString;

	bool closed;
	bool ignoreTth;
	
        CriticalSection cs;

        enum { NUM_SEEKERS = 8 };
        struct SearchData
        {
                SearchData() : curpos(0) { }
                int i;
                string seekers[NUM_SEEKERS];

                void AddSeeker(const string& s)
                {
                        seekers[curpos++] = s;
                        curpos = curpos % NUM_SEEKERS;
                }
        private:
                int curpos;
        };

        typedef HASH_MAP<string, SearchData> SearchMap;
        typedef SearchMap::iterator SearchIter;

        SearchMap searches;

        // ClientManagerListener
        struct SearchInfo
        {
                SearchInfo(const string& _user, const string& _s, ClientManagerListener::SearchReply _re) : seeker(_user), s(_s),  re(_re) { } // !SMT!-S
                string seeker, s;
                ClientManagerListener::SearchReply re; // !SMT!-S
        };

        // void onSearchResult(string aSearchString); // !SMT!-S

  	// ClientManagerListener
        virtual void on(ClientManagerListener::IncomingSearch, const string& user, const string& s, ClientManagerListener::SearchReply re) throw(); // !SMT!-S
	
	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, uint32_t) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

#endif // !defined(SPY_FRAME_H)

/**
 * @file
 * $Id: SpyFrame.h,v 1.4 2008/03/10 07:44:31 alexey Exp $
 */
