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

#if !defined(DIRECTORY_LISTING_FRM_H)
#define DIRECTORY_LISTING_FRM_H

//[-]PPA [Doxygen 1.5.1] #include "../client/User.h"
#include "../client/FastAlloc.h"

#include "TypedListViewCtrl.h"
#include "UCHandler.h"

#include "../client/DirectoryListing.h"
#include "../client/StringSearch.h"
#include "../client/ADLSearch.h"
#include "../client/ShareManager.h" // !PPA!
#include "../peers/TextRenderer.h"
#include "../peers/CaptionFont.h"
#include "../peers/FlatIconButton.h"

class ThreadedDirectoryListing;

#define STATUS_MESSAGE_MAP 9
#define CONTROL_MESSAGE_MAP 10
class DirectoryListingFrame : 
	public MDITabChildWindowImpl<DirectoryListingFrame, IDR_DIRECTORY>, 
	public CSplitterImpl<DirectoryListingFrame>, 
	public UCHandler<DirectoryListingFrame>,
	private SettingsManagerListener
{
private:
	enum { PRIORITY_DIR_OFFSET = 90 };
public:
	static void openWindow(const tstring& aFile, const UserPtr& aUser, int64_t aSpeed);
	static void openWindow(const UserPtr& aUser, const string& txt, int64_t aSpeed);
	static void closeAll();

	typedef MDITabChildWindowImpl<DirectoryListingFrame, IDR_DIRECTORY> baseClass;
	typedef UCHandler<DirectoryListingFrame> ucBase;

	enum {
		COLUMN_FILENAME,
		COLUMN_TYPE,
		COLUMN_EXACTSIZE,
		COLUMN_SIZE,
		COLUMN_TTH,
		COLUMN_PATH, //!PPA!
		COLUMN_LAST
	};

	enum {
		FINISHED,
		ABORTED
	};	

	enum {
		STATUS_TEXT,
		STATUS_SPEED,
		STATUS_TOTAL_FILES,
		STATUS_TOTAL_SIZE,
		STATUS_SELECTED_FILES,
		STATUS_SELECTED_SIZE,
		STATUS_FILE_LIST_DIFF,
		STATUS_MATCH_QUEUE,
		STATUS_FIND,
		STATUS_NEXT,
		STATUS_DUMMY,
		STATUS_LAST
	};

	DirectoryListingFrame(const UserPtr& aUser, int64_t aSpeed);
	~DirectoryListingFrame() { 
		dcassert(lists.find(dl->getUser()) != lists.end());
		lists.erase(dl->getUser());
	}


	DECLARE_FRAME_WND_CLASS(_T("DirectoryListingFrame"), IDR_DIRECTORY)

	BEGIN_MSG_MAP(DirectoryListingFrame)
		NOTIFY_HANDLER(IDC_FILES, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_FILES, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(IDC_FILES, NM_CUSTOMDRAW, onCustomDrawList) // !fulDC!
		NOTIFY_HANDLER(IDC_FILES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_FILES, NM_DBLCLK, onDoubleClickFiles)
		NOTIFY_HANDLER(IDC_FILES, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onSelChangedDirectories)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_CUSTOMDRAW, onCustomDrawTree) // !fulDC!
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE, onOpenFile) // !SMT!-UI
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadDir)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadDirTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_VIDEO, onVideoDownload)
		COMMAND_ID_HANDLER(IDC_GO_TO_DIRECTORY, onGoToDirectory)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_WMLINK, onCopy) // !SMT!-UI
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPM)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy);
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow);
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget);
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET_DIR, IDC_DOWNLOAD_TARGET_DIR + WinUtil::lastDirs.size(), onDownloadTargetDir);
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onDownloadWithPrio);
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED+PRIORITY_DIR_OFFSET, IDC_PRIORITY_HIGHEST+PRIORITY_DIR_OFFSET, onDownloadDirWithPrio);
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadFavoriteDirs);
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadWholeFavoriteDirs);
		CHAIN_COMMANDS(ucBase);
		CHAIN_MSG_MAP(baseClass);
		CHAIN_MSG_MAP(CSplitterImpl<DirectoryListingFrame>);
		REFLECT_NOTIFICATIONS();
	ALT_MSG_MAP(STATUS_MESSAGE_MAP);
		COMMAND_ID_HANDLER(IDC_FIND, onFind);
		COMMAND_ID_HANDLER(IDC_NEXT, onNext);
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue);
		COMMAND_ID_HANDLER(IDC_FILELIST_DIFF, onListDiff);
	ALT_MSG_MAP(CONTROL_MESSAGE_MAP);
		MESSAGE_HANDLER(WM_XBUTTONUP, onXButtonUp);
	END_MSG_MAP()

	LRESULT onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/); // !SMT!-UI
		LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
		LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
		LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadDirWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadDirTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onVideoDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadTargetDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDoubleClickFiles(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
		LRESULT onSelChangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
		LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
		LRESULT onXButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
		LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
		LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/); 
		LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/); // !fulDC!
		LRESULT onCustomDrawTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/); // !fulDC!

		void downloadList(const tstring& aTarget, bool view = false,  QueueItem::Priority prio = QueueItem::DEFAULT);
		void updateTree(DirectoryListing::Directory* tree, HTREEITEM treeItem);
		void UpdateLayout(BOOL bResizeBars = TRUE);
		void findFile(bool findNext);
		void runUserCommand(UserCommand& uc);
		void loadFile(const tstring& name);
		void loadXML(const string& txt);
		void refreshTree(const tstring& root);

		HTREEITEM findItem(HTREEITEM ht, const tstring& name);
		void selectItem(const tstring& name);

		LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
			updateStatus();
			return 0;
		}

		LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
			ctrlList.SetFocus();
			return 0;
		}

		void setWindowTitle() {
			if(error.empty())
				SetWindowText((Text::toT(dl->getUser()->getFirstNick()) + _T(" - ") + WinUtil::getHubNames(dl->getUser()).first).c_str());
			else
				SetWindowText(error.c_str());		
		}

		LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
			return 1;
		}

		void clearList() {
			int j = ctrlList.GetItemCount();
			for(int i = 0; i < j; i++) {
				delete (ItemInfo*)ctrlList.GetItemData(i);
			}
			ctrlList.DeleteAllItems();
		}

		LRESULT onFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
			searching = true;
			findFile(false);
			searching = false;
			updateStatus();
			return 0;
		}
		LRESULT onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
			searching = true;
			findFile(true);
			searching = false;
			updateStatus();
			return 0;
		}

		LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT onListDiff(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

		LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

		LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
			NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
			if(kd->wVKey == VK_TAB) {
				onTab();
			}
			return 0;
		}

		void onTab() {
			HWND focus = ::GetFocus();
			if(focus == ctrlTree.m_hWnd) {
				ctrlList.SetFocus();
			} else if(focus == ctrlList.m_hWnd) {
				ctrlTree.SetFocus();
			}
		}

		LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
			PostMessage(WM_CLOSE);
			return 0;
		}

private:
	friend class ThreadedDirectoryListing;

	void appendFavoriteDirs(OMenu& menu, int command);
	void appendLastDirs(OMenu& menu, int command, int startIndex = 0);

	void getItemColor(const Flags::MaskType flags, COLORREF &fg, COLORREF &bg); // !SMT!-UI
	void changeDir(DirectoryListing::Directory* d, BOOL enableRedraw);
	HTREEITEM findFile(const StringSearch& str, HTREEITEM root, int &foundFile, int &skipHits);
	void updateStatus();
	void initStatus();
	void addHistory(const string& name);
	void up();
	void back();
	void forward();
	bool hasSelectedFiles();

	class ItemInfo : public FastAlloc<ItemInfo> {
	public:
		enum ItemType {
			FILE,
			DIRECTORY
		} type;

		union {
			DirectoryListing::File* file;
			DirectoryListing::Directory* dir;
		};

		const tstring& getText(int col) const {
			return columns[col];
		}

		struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize() : a->file->getSize(); }
			int64_t total;
		};

		ItemInfo(DirectoryListing::File* f);
		ItemInfo(DirectoryListing::Directory* d);

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
			if(a->type == DIRECTORY) {
				if(b->type == DIRECTORY) {
					switch(col) 
					{
					case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					case COLUMN_SIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					default: return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
					}
				} else {
					return -1;
				}
			} else if(b->type == DIRECTORY) {
				return 1;
			} else {
				switch(col) 
				{
				case COLUMN_EXACTSIZE: return compare(a->file->getSize(), b->file->getSize());
				case COLUMN_SIZE: return compare(a->file->getSize(), b->file->getSize());
				default: return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str(),false);
				}
			}
		}
		int imageIndex() const {
			if(type == DIRECTORY)
				return WinUtil::getDirIconIndex();
			else
				return WinUtil::getIconIndex(getText(COLUMN_FILENAME));
		}

	private:
		tstring columns[COLUMN_LAST];
	};

	void prepareFileMenuTop(OMenu &fileMenu, DirectoryListing::File *aFile);
	void prepareFileMenuBottom(OMenu &fileMenu);

	OMenu targetMenu;
	OMenu targetDirMenu;
	OMenu directoryMenu;
	OMenu priorityMenu;
	OMenu priorityDirMenu;
	OMenu copyMenu;
	OMenu tabMenu;

	void createCopyMenu(bool fileMode);

	CContainedWindow statusContainer;
	CContainedWindow treeContainer;
	CContainedWindow listContainer;

	StringList targets;

	deque<string> history;
	size_t historyIndex;

	CTreeViewCtrl ctrlTree;
	TypedListViewCtrl<ItemInfo, IDC_FILES> ctrlList;
	CStatusBarCtrl ctrlStatus;
	HTREEITEM treeRoot;

	CButton ctrlFind, ctrlFindNext;
	CButton ctrlListDiff;
	CButton ctrlMatchQueue;

	string findStr;
	tstring error;
	string size;

	int skipHits;

	size_t files;
	int64_t speed;		/**< Speed at which this file list was downloaded */
	int m_headerHeight;
	TextRenderer::TextBlock m_header;
	CaptionFont m_headerFont;
	FlatIconButton m_userAddToFriends;
	FlatIconButton m_sendPrivateMessage;
	FlatIconButton m_downloadFile;
	FlatIconButton m_downloadFolder;

	bool updating;
	bool searching;
	bool loading;

	int statusSizes[10];

	auto_ptr<DirectoryListing> dl;

	StringMap ucLineParams;

	typedef HASH_MAP_X(UserPtr, DirectoryListingFrame*, User::HashFunction, equal_to<UserPtr>, less<UserPtr>) UserMap;
	typedef UserMap::const_iterator UserIter;

	static UserMap lists;

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	typedef map< HWND , DirectoryListingFrame* > FrameMap;
	typedef pair< HWND , DirectoryListingFrame* > FramePair;
	typedef FrameMap::iterator FrameIter;

	static FrameMap frames;

	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

class ThreadedDirectoryListing : public Thread
{
public:
	ThreadedDirectoryListing(DirectoryListingFrame* pWindow, 
		const string& pFile, const string& pTxt) : mWindow(pWindow),
		mFile(pFile), mTxt(pTxt)
	{ }

protected:
	DirectoryListingFrame* mWindow;
	string mFile;
	string mTxt;

private:
	int run()
	{
		try
		{
			if(!mFile.empty()) {
				mWindow->dl->loadFile(mFile);
				ADLSearchManager::getInstance()->matchListing(*mWindow->dl);
				string filename = Util::getFileName(mFile);
				if( _strnicmp(filename.c_str(), "files", 5) // !SMT!-UI
					|| _strnicmp(filename.c_str()+filename.length()-8, ".xml.bz2", 8))
					mWindow->dl->checkDupes(); // !fulDC!
				mWindow->refreshTree(Text::toT(WinUtil::getInitialDir(mWindow->dl->getUser())));
			} else {
				mWindow->refreshTree(Text::toT(Util::toNmdcFile(mWindow->dl->loadXML(mTxt, true))));
			}

			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::FINISHED);
		}catch(const AbortException& ) {
			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
		} catch(const Exception& e) {
			mWindow->error = Text::toT(mWindow->dl->getUser()->getFirstNick() + ": " + e.getError());
			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
		}

		//cleanup the thread object
		delete this;

		return 0;
	}
};

#endif // !defined(DIRECTORY_LISTING_FRM_H)

/**
* @file
* $Id: DirectoryListingFrm.h,v 1.9.2.1 2008/11/04 14:09:33 alexey Exp $
*/
