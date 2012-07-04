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

#if !defined(QUEUE_FRAME_H)
#define QUEUE_FRAME_H

#include "TypedListViewCtrl.h"

#include "../client/QueueManager.h"
#include "../client/FastAlloc.h"
#include "../client/TaskQueue.h"
#include "../client/FileChunksInfo.h"

#include "../peers/FrameHeader.h"
#include "../peers/FlatIconButton.h"
#include "../peers/FlatMultiIconButton.h"
#include "../peers/PrioritySlider.h"
#include "../peers/CaptionFont.h"

#define SHOWTREE_MESSAGE_MAP 12

class QueueFrame : 
  public MDITabChildWindowImpl<QueueFrame, IDI_PEERS_DOWNLOAD_QUEUE>,
  public StaticFrame<QueueFrame, ResourceManager::DOWNLOAD_QUEUE_TITLE, IDC_QUEUE>,
  private QueueManagerListener,
  public CSplitterImpl<QueueFrame>,
  private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame"), IDI_PEERS_DOWNLOAD_QUEUE, 0, COLOR_3DFACE);

	QueueFrame() : menuItems(0), queueSize(0), queueItems(0), spoken(false), dirty(false), 
		usingDirMenu(false),  readdItems(0), fileLists(NULL), showTree(true), PreviewAppsSize(0),
                m_priorityLabelFont(WinUtil::systemFont, 7, 8),
		showTreeContainer(WC_BUTTON, this, SHOWTREE_MESSAGE_MAP) 
	{
	}

        virtual bool isLargeIcon() const { return true; }

	~QueueFrame() {
		// Clear up dynamicly allocated menu objects
		browseMenu.ClearMenu();
		removeMenu.ClearMenu();
		removeAllMenu.ClearMenu();
		pmMenu.ClearMenu();
		readdMenu.ClearMenu();		
	}
	
	typedef MDITabChildWindowImpl<QueueFrame, IDI_PEERS_DOWNLOAD_QUEUE> baseClass;
	typedef CSplitterImpl<QueueFrame> splitBase;

        BEGIN_MSG_MAP(QueueFrame)
                NOTIFY_HANDLER(IDC_QUEUE, LVN_GETDISPINFO, ctrlQueue.onGetDispInfo)
                NOTIFY_HANDLER(IDC_QUEUE, LVN_COLUMNCLICK, ctrlQueue.onColumnClick)
                NOTIFY_HANDLER(IDC_QUEUE, LVN_GETINFOTIP, ctrlQueue.onInfoTip)
                NOTIFY_HANDLER(IDC_QUEUE, LVN_KEYDOWN, onKeyDown)
                NOTIFY_HANDLER(IDC_QUEUE, LVN_ITEMCHANGED, onItemChangedQueue)
                NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onItemChanged)
                NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
                NOTIFY_HANDLER(IDC_QUEUE, NM_DBLCLK, onSearchDblClick) // !SMT!-UI
                MESSAGE_HANDLER(WM_CREATE, OnCreate)
                MESSAGE_HANDLER(WM_CLOSE, onClose)
                MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
                MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
                MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
                MESSAGE_HANDLER(WPM_IDLE, onIdle)
                COMMAND_HANDLER(IDC_DOWNLOAD_START_PAUSE, BN_CLICKED, onStartPauseButton);
                COMMAND_HANDLER(IDC_DOWNLOAD_DELETE, BN_CLICKED, onDeleteButton);
                COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
                COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_COPY_WMLINK, onCopy) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
                COMMAND_ID_HANDLER(IDC_REMOVE_OFFLINE, onRemoveOffline)
                COMMAND_ID_HANDLER(IDC_MOVE, onMove)
                COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + COLUMN_LAST-1, onCopy)
                COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onPriority)
                COMMAND_RANGE_HANDLER(IDC_SEGMENTONE, IDC_SEGMENTTWO_HUNDRED, onSegments) // !BUGMASTER!-S
		COMMAND_RANGE_HANDLER(IDC_BROWSELIST, IDC_BROWSELIST + menuItems, onBrowseList)
		COMMAND_RANGE_HANDLER(IDC_REMOVE_SOURCE, IDC_REMOVE_SOURCE + menuItems, onRemoveSource)
		COMMAND_RANGE_HANDLER(IDC_REMOVE_SOURCES, IDC_REMOVE_SOURCES + 1 + menuItems, onRemoveSources)
		COMMAND_RANGE_HANDLER(IDC_PM, IDC_PM + menuItems, onPM)
		COMMAND_RANGE_HANDLER(IDC_READD, IDC_READD + 1 + readdItems, onReadd)
		COMMAND_ID_HANDLER(IDC_AUTOPRIORITY, onAutoPriority)
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
		NOTIFY_HANDLER(IDC_QUEUE, NM_CUSTOMDRAW, onCustomDraw)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	END_MSG_MAP()

	LRESULT onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveSource(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveSources(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReadd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onAutoPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onIdle(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onStartPauseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDeleteButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void removeDir(HTREEITEM ht);
	void setPriority(HTREEITEM ht, const QueueItem::Priority& p);
	void setAutoPriority(HTREEITEM ht, const bool& ap);
	void changePriority(bool inc);

	LRESULT onItemChangedQueue(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* lv = (NMLISTVIEW*)pnmh;
		if((lv->uNewState & LVIS_SELECTED) != (lv->uOldState & LVIS_SELECTED))
			updateStatus();
		return 0;
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlQueue.SetFocus();
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		usingDirMenu ? removeSelectedDir() : removeSelected();
		return 0;
	}

	LRESULT onMove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		usingDirMenu ? moveSelectedDir() : moveSelected();
		return 0;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} else if(kd->wVKey == VK_ADD){
			// Increase Item priority
			changePriority(true);
		} else if(kd->wVKey == VK_SUBTRACT){
			// Decrease item priority
			changePriority(false);
		} else if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

        LRESULT onSearchDblClick(int idCtrl, LPNMHDR /*pnmh*/, BOOL& bHandled) {
			return onSearchAlternates(BN_CLICKED, (WORD)idCtrl, m_hWnd, bHandled); // !SMT!-UI
		}

        LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
                NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
                if(kd->wVKey == VK_DELETE) {
                        removeSelectedDir();
                } else if(kd->wVKey == VK_TAB) {
                        onTab();
                }
                return 0;
        }

	void onTab();

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}
	
private:

	enum {
		COLUMN_FIRST,
		COLUMN_TARGET = COLUMN_FIRST,
		COLUMN_STATUS,
		COLUMN_SEGMENTS,
		COLUMN_SIZE,
		COLUMN_PROGRESS,
		COLUMN_DOWNLOADED,
		COLUMN_PRIORITY,
		COLUMN_USERS,
		COLUMN_PATH,
		COLUMN_EXACT_SIZE,
		COLUMN_ERRORS,
		COLUMN_ADDED,
		COLUMN_TTH,
		COLUMN_TYPE,
		COLUMN_LAST
	};
	enum Tasks {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM
	};
	
	class QueueItemInfo;
	friend class QueueItemInfo;
	
	class QueueItemInfo : public Flags, public FastAlloc<QueueItemInfo> {
			tstring m_columns[COLUMN_LAST];
	public:
		enum {
			MASK_TARGET = 1 << COLUMN_TARGET,
			MASK_STATUS = 1 << COLUMN_STATUS,
			MASK_SEGMENTS = 1 << COLUMN_SEGMENTS,
			MASK_SIZE = 1 << COLUMN_SIZE,
			MASK_DOWNLOADED = 1 << COLUMN_DOWNLOADED,
			MASK_PRIORITY = 1 << COLUMN_PRIORITY,
			MASK_USERS = 1 << COLUMN_USERS,
			MASK_PATH = 1 << COLUMN_PATH,
			MASK_ERRORS = 1 << COLUMN_ERRORS,
			MASK_ADDED = 1 << COLUMN_ADDED,
			MASK_TTH = 1 << COLUMN_TTH,
			MASK_TYPE = 1 << COLUMN_TYPE
		};

		QueueItemInfo(const QueueItem& aQI) : Flags(aQI), chunkInfo(aQI.getChunkInfo()),
			target(aQI.getTarget()), autoPriority(aQI.getAutoPriority()),
			size(aQI.getSize()), downloadedBytes(aQI.getDownloadedBytes()), 
			added(aQI.getAdded()), tth(aQI.getTTH()), priority(aQI.getPriority()), status(aQI.getStatus()),
			updateMask((uint32_t)-1), sources(aQI.getSources()), badSources(aQI.getBadSources())
		{ 
		}

		~QueueItemInfo() { }

		void update();

		void remove() { QueueManager::getInstance()->remove(getTarget()); }

		// TypedListViewCtrl functions
		const tstring& getText(int col) const{
			return m_columns[col];
		}
		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
			switch(col) {
				case COLUMN_SIZE: case COLUMN_EXACT_SIZE: return compare(a->getSize(), b->getSize());
				case COLUMN_PRIORITY: return compare((int)a->getPriority(), (int)b->getPriority());
				case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
				case COLUMN_ADDED: return compare(a->getAdded(), b->getAdded());
				default: 
					return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}
		int imageIndex() const { return WinUtil::getIconIndex(Text::toT(getTarget()));	}

		QueueItem::SourceList& getSources() { return sources; }
		QueueItem::SourceList& getBadSources() { return badSources; }
		string getPath() { return Util::getFilePath(getTarget()); }


		bool isSource(const UserPtr& u) {
			return find(sources.begin(), sources.end(), u) != sources.end();
		}
		bool isBadSource(const UserPtr& u) {
			return find(badSources.begin(), badSources.end(), u) != badSources.end();
		}
		
		GETSET(string, target, Target);
		GETSET(int64_t, size, Size);
		GETSET(int64_t, downloadedBytes, DownloadedBytes);
		GETSET(time_t, added, Added);
		GETSET(QueueItem::Priority, priority, Priority);
		GETSET(QueueItem::Status, status, Status);
		GETSET(TTHValue, tth, TTH);
		GETSET(QueueItem::SourceList, sources, Sources);
		GETSET(QueueItem::SourceList, badSources, BadSources);
		GETSET(bool, autoPriority, AutoPriority);
		uint32_t updateMask;
		FileChunksInfo::Ptr chunkInfo;
	
	private:

		QueueItemInfo(const QueueItemInfo&);
		QueueItemInfo& operator=(const QueueItemInfo&);
	};
		
	struct QueueItemInfoTask : FastAlloc<QueueItemInfoTask>, public Task {
		QueueItemInfoTask(QueueItemInfo* ii_) : ii(ii_) { }
		QueueItemInfo* ii;
	};

	struct UpdateTask : 
		//FastAlloc<UpdateTask>, 
		public Task {
		UpdateTask(const QueueItem& source) : target(source.getTarget()), priority(source.getPriority()), size(source.getSize()),
			status(source.getStatus()), downloadedBytes(source.getDownloadedBytes()), sources(source.getSources()), badSources(source.getBadSources()) 
		{
		}

		string target;
		QueueItem::Priority priority;
		QueueItem::Status status;
		int64_t downloadedBytes;
		int64_t size;

		QueueItem::SourceList sources;
		QueueItem::SourceList badSources;
	};

	TaskQueue tasks;
	bool spoken;
        FrameHeader m_header;
        FlatMultiIconButton m_btnPause;
        FlatIconButton m_btnDelete;
        PrioritySlider m_prioritySlider;
        const TCHAR* getPriorityName(QueueItem::Priority priority) const;
        CStatic m_priorityLabel;
        CStatic m_priorityValue;
        CaptionFont m_priorityLabelFont;

	/** Single selection in the queue part */
	OMenu singleMenu;
	/** Multiple selection in the queue part */
	OMenu multiMenu;
	/** Tree part menu */
	OMenu browseMenu;

	OMenu removeMenu;
	OMenu removeAllMenu;
	OMenu pmMenu;
	OMenu priorityMenu;
	OMenu readdMenu;
	OMenu dirMenu;
	OMenu previewMenu;
	OMenu segmentsMenu;
	OMenu copyMenu;

	int PreviewAppsSize;

	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	bool showTree;

	bool usingDirMenu;

	bool dirty;

	int menuItems;
	int readdItems;

	HTREEITEM fileLists;

	typedef HASH_MULTIMAP_X(string, QueueItemInfo*, noCaseStringHash, noCaseStringEq, noCaseStringLess) DirectoryMap;
	typedef DirectoryMap::iterator DirectoryIter;
	typedef pair<DirectoryIter, DirectoryIter> DirectoryPair;
	DirectoryMap directories;
	string curDir;

	TypedListViewCtrl<QueueItemInfo, IDC_QUEUE> ctrlQueue;
        typedef TypedListViewCtrl<QueueItemInfo, IDC_QUEUE>::Selection Selection;
	CTreeViewCtrl ctrlDirs;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[6];
	
	int64_t queueSize;
	int queueItems;

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	void addQueueList(const QueueItem::StringMap& l);
	void addQueueItem(QueueItemInfo* qi, bool noSort);
	HTREEITEM addDirectory(const string& dir, bool isFileList = false, HTREEITEM startAt = NULL);
	void removeDirectory(const string& dir, bool isFileList = false);
	void removeDirectories(HTREEITEM ht);

	void updateQueue();
	void updateStatus();
	
	/**
	 * This one is different from the others because when a lot of files are removed
	 * at the same time, the WM_SPEAKER messages seem to get lost in the handling or
	 * something, they're not correctly processed anyway...thanks windows.
	 */
	void speak(Tasks t, Task* p) {
        tasks.add(static_cast<uint8_t>(t), p);
		if(!spoken) {
			spoken = true;
			PostMessage(WM_SPEAKER);
		}
	}

	bool isCurDir(const string& aDir) const { return Util::stricmp(curDir, aDir) == 0; }

	void moveSelected();	
	void moveSelectedDir();
	void moveDir(HTREEITEM ht, const string& target);

	void moveNode(HTREEITEM item, HTREEITEM parent);

	void clearTree(HTREEITEM item);

	QueueItemInfo* getItemInfo(const string& target);

	void removeSelected();
	void removeSelectedDir();
	
	const string& getSelectedDir() {
		HTREEITEM ht = ctrlDirs.GetSelectedItem();
		return ht == NULL ? Util::emptyString : getDir(ctrlDirs.GetSelectedItem());
	}
	
	const string& getDir(HTREEITEM ht) { dcassert(ht != NULL); return *reinterpret_cast<string*>(ctrlDirs.GetItemData(ht)); }

	virtual void on(QueueManagerListener::Added, QueueItem* aQI) throw();
	virtual void on(QueueManagerListener::Moved, QueueItem* aQI, const string& oldTarget) throw();
	virtual void on(QueueManagerListener::Removed, QueueItem* aQI) throw();
	virtual void on(QueueManagerListener::SourcesUpdated, QueueItem* aQI) throw();
	virtual void on(QueueManagerListener::StatusUpdated, QueueItem* aQI) throw() { on(QueueManagerListener::SourcesUpdated(), aQI); }
	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

#endif // !defined(QUEUE_FRAME_H)

/**
 * @file
 * $Id: QueueFrame.h,v 1.10 2008/03/31 15:25:39 alexey Exp $
 */
