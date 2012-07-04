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

#if !defined(SEARCH_FRM_H)
#define SEARCH_FRM_H

#include "TypedListViewCtrl.h"
#include "UserInfoBase.h"

#include "../client/SearchManager.h"
#include "../client/CriticalSection.h"
#include "../client/pme.h"

#include "UCHandler.h"
#include "../peers/SearchFrmFactory.h"
#include "../peers/FilterEdit.h"
#include "../peers/TextRenderer.h"
#include "../peers/CaptionFont.h"

class SearchFrameHeader : public CWindowImpl<SearchFrameHeader>, private TextRenderer::Paragraph {
private:
  friend class SearchFrame;
  CaptionFont captionFont;
  CaptionFont textFont;
  int m_textHeight;
public:
  SearchFrameHeader(): captionFont(CaptionFont::BOLD), textFont(CaptionFont::BOLD,5,4) {
  }

  BEGIN_MSG_MAP(SearchFrameHeader)
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
  END_MSG_MAP();
    
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    CPaintDC dc(m_hWnd);
    CRect r;
    GetClientRect(r);
    dc.FillRect(r, GetSysColorBrush(COLOR_3DFACE));
    dc.SetBkColor(GetSysColor(COLOR_3DFACE));    
    draw((HDC) dc, 8, max(0, r.Height() - m_textHeight) / 2);
    return 0;
  }

  bool updateLayout(CDCHandle dc, int paragraphWidth, int x, int y, int height) {
    CRect r;
    GetClientRect(r);
    MapWindowPoints(GetParent(), r);
    m_textHeight = doLayout(dc, paragraphWidth);
    if (height != r.Height() || paragraphWidth != r.Width() || x != r.left || y != r.top) {
      MoveWindow(x, y, paragraphWidth, height);
      return true;
    }
    else {
      return false;
    }
  }

  void updateLayout() {
    CRect r;
    GetClientRect(r);
    CClientDC dc(m_hWnd);
    m_textHeight = doLayout((HDC) dc, r.Width());
    Invalidate();
  }

  void setText(const TextRenderer::WordKey& wordKey, const tstring& text) {
    if (TextRenderer::Paragraph::setText(wordKey, text)) {
      updateLayout();
    }
  }

};

class SearchFrame : 
  public MDITabChildWindowImpl<SearchFrame, IDI_PEERS_SEARCH>, 
  private SearchManagerListener, 
  public UCHandler<SearchFrame>, 
  public UserInfoBaseHandler<SearchFrame>,
  private SettingsManagerListener, 
  private TimerManagerListener
{
public:

	DECLARE_FRAME_WND_CLASS_EX(_T("SearchFrame"), IDI_PEERS_SEARCH, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SearchFrame, IDI_PEERS_SEARCH> baseClass;
	typedef UCHandler<SearchFrame> ucBase;
	typedef UserInfoBaseHandler<SearchFrame> uicBase;

        BEGIN_MSG_MAP(SearchFrame)
                NOTIFY_HANDLER(IDC_RESULTS, LVN_GETDISPINFO, ctrlResults.onGetDispInfo)
                NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, ctrlResults.onColumnClick)
                NOTIFY_HANDLER(IDC_RESULTS, LVN_GETINFOTIP, ctrlResults.onInfoTip)
                NOTIFY_HANDLER(IDC_RESULTS, NM_DBLCLK, onDoubleClickResults)
                NOTIFY_HANDLER(IDC_RESULTS, LVN_KEYDOWN, onKeyDown)
                NOTIFY_HANDLER(IDC_RESULTS, NM_CUSTOMDRAW, onCustomDraw)
                MESSAGE_HANDLER(WM_CREATE, onCreate)
                MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
                MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
                MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
                MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
                MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColorStatic)
                MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
                MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
                COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
                COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
                COMMAND_ID_HANDLER(IDC_SEARCH_PAUSE, onPause)
                COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy)
                COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy)
                COMMAND_ID_HANDLER(IDC_COPY_PATH, onCopy)
                COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy)
                COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
                COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
                COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
#ifdef PPA_INCLUDE_BITZI_LOOKUP
                COMMAND_ID_HANDLER(IDC_BITZI_LOOKUP, onBitziLookup)
#endif
                COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
                COMMAND_ID_HANDLER(IDC_COPY_WMLINK, onCopy) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)

                COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
				COMMAND_ID_HANDLER(IDC_VIDEO, onVideoDownload)
                COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + 499, onDownload)
                COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + 499, onDownloadWhole)
                COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + 499, onDownloadTarget)
                COMMAND_HANDLER(IDC_SEARCH_FILTER, EN_CHANGE, onFilterChange);
                COMMAND_HANDLER(IDC_SEARCH_FILTER_MODE, CBN_SELCHANGE, onSelChange);

                CHAIN_COMMANDS(ucBase)
                CHAIN_COMMANDS(uicBase)
                CHAIN_MSG_MAP(baseClass)
        END_MSG_MAP()

	SearchFrame();
	virtual ~SearchFrame();
        virtual bool isLargeIcon() const { return true; }

        LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT onClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
        LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT onCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT onDoubleClickResults(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
        LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
#ifdef PPA_INCLUDE_BITZI_LOOKUP
        LRESULT onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
#endif
        LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT onFilterChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

        LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onVideoDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onDownloadWhole(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

        void UpdateLayout(BOOL bResizeBars = TRUE);
        void runUserCommand(UserCommand& uc);

	void removeSelected() {
		int i = -1;
		Lock l(cs);
		while( (i = ctrlResults.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ctrlResults.removeGroupedItem(ctrlResults.getItemData(i));
		}
	}
	
	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::view);
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		removeSelected();
		return 0;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} 
		return 0;
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlResults.SetFocus();
		return 0;
	}

        void initialize(const SearchFrameFactory::Request& request);
        virtual void onActivate();

	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
private:
        int windowActivationCount;
        tstring getDownloadDirectory(WORD wID);
        class SearchInfo;

public:
	TypedTreeListViewCtrl<SearchInfo, IDC_RESULTS>& getUserList() { return ctrlResults; }

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_LOCAL_PATH,
		COLUMN_HITS,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
//[-]PPA		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
                COLUMN_LOCATION, // !SMT!-IP
		COLUMN_IP,		
#ifdef PPA_INCLUDE_DNS
                COLUMN_DNS, // !SMT!-IP
#endif
		COLUMN_TTH,
		COLUMN_LAST
	};

	enum Images {
		IMAGE_UNKOWN,
		IMAGE_SLOW,
		IMAGE_NORMAL,
		IMAGE_FAST
	};

	enum FilterModes{
		NONE,
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	class SearchInfo : public UserInfoBase {
	public:
		typedef SearchInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::const_iterator Iter;

		SearchInfo::List subItems;

		SearchInfo(SearchResult* aSR) : UserInfoBase(aSR->getUser()), sr(aSR), collapsed(true), main(NULL) { 
			sr->incRef(); update();
		}
		~SearchInfo() {
			sr->decRef(); 
		}

		bool collapsed;
		SearchInfo* main;

		void getList();
		void browseList();

		void view();
		struct Download {
			Download(const tstring& aTarget) : tgt(aTarget) { }
			void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct DownloadWhole {
			DownloadWhole(const tstring& aTarget) : tgt(aTarget) { }
			void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct DownloadTarget {
			DownloadTarget(const tstring& aTarget) : tgt(aTarget) { }
			void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct CheckTTH {
			CheckTTH() : op(true), firstHubs(true), hasTTH(false), firstTTH(true) { }
			void operator()(SearchInfo* si);
			bool firstHubs;
			StringList hubs;
			bool op;
			bool hasTTH;
			bool firstTTH;
			tstring tth;
		};
        
		const tstring& getText(int col) const { return columns[col]; }

		static int compareItems(const SearchInfo* a, const SearchInfo* b, int col) {
			if(!a->sr || !b->sr)
				return 0;

			switch(col) {
				case COLUMN_TYPE: 
					if(a->sr->getType() == b->sr->getType())
						return lstrcmpi(a->columns[COLUMN_TYPE].c_str(), b->columns[COLUMN_TYPE].c_str());
					else
						return(a->sr->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
				case COLUMN_HITS: return compare(a->subItems.size(), b->subItems.size());
				case COLUMN_SLOTS: 
					if(a->sr->getFreeSlots() == b->sr->getFreeSlots())
						return compare(a->sr->getSlots(), b->sr->getSlots());
					else
						return compare(a->sr->getFreeSlots(), b->sr->getFreeSlots());
				case COLUMN_SIZE:
				case COLUMN_EXACT_SIZE: return compare(a->sr->getSize(), b->sr->getSize());
//				case COLUMN_UPLOAD: return compare(a->columns[COLUMN_UPLOAD],b->columns[COLUMN_UPLOAD]);
				default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}

		int imageIndex() const {
			return sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getIconIndex(Text::toT(sr->getFile())) : WinUtil::getDirIconIndex();
		}

		void update();
		
		SearchInfo* createMainItem() { return this; }
		const tstring& getGroupingString() const { return columns[COLUMN_TTH]; }
		void updateMainItem() {
			uint32_t total = main->subItems.size();
			if(total != 0) {
				TCHAR buf[256];
				snwprintf(buf, sizeof(buf), _T("%d %s"), total + 1, CTSTRING(USERS));

				main->columns[COLUMN_HITS] = buf;
				if(total == 1)
					main->columns[COLUMN_SIZE] = columns[COLUMN_SIZE];
			} else {
				main->columns[COLUMN_HITS] = Util::emptyStringT;
			}
		}

                GETSET(int, flagimage, FlagImage);
		SearchResult* sr;
		tstring columns[COLUMN_LAST];
	};

	// WM_SPEAKER
	enum Speakers {
		ADD_RESULT,
		FILTER_RESULT,
		QUEUE_STATS,
		SEARCH_START
	};

	tstring initialString;
	int64_t initialSize;
	SearchManager::SizeModes initialMode;
	SearchManager::TypeModes initialType;

	CButton ctrlPauseSearch;

	tstring filter;
	
        CStatic srLabel;
        int m_headerHeight;

        CImageList images;
        TypedTreeListViewCtrl<SearchInfo, IDC_RESULTS> ctrlResults;

        // OMenu grantMenu; // !SMT!-UI
	OMenu resultsMenu;
	OMenu targetMenu;
	OMenu targetDirMenu;
	OMenu copyMenu;
	
	TStringList search;
	StringList targets;
	StringList wholeTargets;
	SearchInfo::List PausedResults;

	CFilterEdit ctrlFilter;
	CComboBox ctrlFilterSel;
        SearchFrameHeader m_header;
	size_t m_initialWordCount; 
        TextRenderer::WordKey m_wordSearchText;
        TextRenderer::WordKey m_wordFileCount;

	bool onlyFree;
	bool isHash;
	bool expandSR;
	bool bPaused;
	bool exactSize1;
	const bool useGrouping;
	int64_t exactSize2;
	int64_t resultsCount;

	CriticalSection cs;

	size_t droppedResults;

	int searches;

	StringMap ucLineParams;
		
	static int columnIndexes[];
	static int columnSizes[];

        typedef map<int,tstring> TargetsMap; // !SMT!-S
        TargetsMap dlTargets; // !SMT!-S

	void updateSearchText(const tstring& searchText);
	void updateSearchResultCount();
	void downloadSelected(const tstring& aDir, bool view = false); 
	void downloadWholeSelected(const tstring& aDir);
	void onEnter();
	void onTab(bool shift);
        void clearPausedResults();

	void download(SearchResult* aSR, const tstring& aDir, bool view);
	
	virtual void on(SearchManagerListener::SR, SearchResult* aResult) throw();
	virtual void on(SearchManagerListener::Searching, SearchQueueItem* aSearch) throw();

	virtual void on(TimerManagerListener::Second, uint32_t aTick) throw();

	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

        bool matchFilter(SearchInfo* si, int sel, bool doSizeCompare = false, FilterModes mode = NONE, int64_t size = 0, PME* filter = NULL);
	bool parseFilter(FilterModes& mode, int64_t& size);
	void updateSearchList(SearchInfo* si = NULL);

};

#endif // !defined(SEARCH_FRM_H)

/**
 * @file
 * $Id: SearchFrm.h,v 1.32.2.1 2008/12/17 19:28:04 alexey Exp $
 */

