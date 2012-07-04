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

#if !defined(TRANSFER_VIEW_H)
#define TRANSFER_VIEW_H

#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/ConnectionManagerListener.h"
#include "../client/TaskQueue.h"
#include "../client/ConnectionManager.h"
#include "../client/HashManager.h"

#include "UCHandler.h"
#include "TypedListViewCtrl.h"
#include "UserInfoBase.h"
#include "../peers/ImageListContainer.h"
#include "../peers/FlatButton.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>,
	private SettingsManagerListener
{
public:
	DECLARE_WND_CLASS(_T("TransferView"))

	TransferView();
	~TransferView(void);

	typedef UserInfoBaseHandler<TransferView> uibBase;
	typedef UCHandler<TransferView> ucBase;

	BEGIN_MSG_MAP(TransferView)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETDISPINFO, ctrlTransfers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_COLUMNCLICK, ctrlTransfers.onColumnClick)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETINFOTIP, ctrlTransfers.onInfoTip)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_KEYDOWN, onKeyDownTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_DBLCLK, onDoubleClickTransfers)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
                MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
                MESSAGE_HANDLER(WM_LBUTTONUP, onLButtonUp)
		COMMAND_ID_HANDLER(IDC_FORCE, onForce)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_CONNECT_ALL, onConnectAll)
		COMMAND_ID_HANDLER(IDC_DISCONNECT_ALL, onDisconnectAll)
		COMMAND_ID_HANDLER(IDC_COLLAPSE_ALL, onCollapseAll)
		COMMAND_ID_HANDLER(IDC_EXPAND_ALL, onExpandAll)
#ifdef PPA_INCLUDE_DROP_SLOW
		COMMAND_ID_HANDLER(IDC_MENU_SLOWDISCONNECT, onSlowDisconnect)
#endif
                COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_COPY_WMLINK, onCopy) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_TRANSFER_VIEW_TOGGLE, onToggle) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_PRIORITY_PAUSED, onPause)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
                COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + COLUMN_LAST-1, onCopy) // !SMT!-UI
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)

                // !SMT!-S
                COMMAND_ID_HANDLER(IDC_SPEED_NONE, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_SUPER,onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_BAN, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_02K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_05K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_08K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_12K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_16K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_24K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_32K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_64K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_128K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_256K, onSetUserLimit)
                // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_ASK_SLOT, onAskSlot)
                REFLECT_NOTIFICATIONS();

		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
	END_MSG_MAP()

        LRESULT onAskSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/); // !SMT!-UI
        LRESULT onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/); // !SMT!-UI
        LRESULT onToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
        LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
        LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);			
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onConnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
#ifdef PPA_INCLUDE_DROP_SLOW
	LRESULT onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
#endif
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
//[-]PPA	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        // !SMT!-S
        LRESULT onSetUserLimit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void runUserCommand(UserCommand& uc);
	void prepareClose();

	LRESULT onCollapseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CollapseAll();
		return 0;
	}

	LRESULT onExpandAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ExpandAll();
		return 0;
	}

	LRESULT onKeyDownTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		}
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		return 0;
	}

	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::setPriorityPause);
		return 0;
	}

	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeAll);
		return 0;
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlTransfers.deleteAllItems();
		return 0;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif		
	}

        LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:
        int m_headerHeight; // высота заголовка панели
        bool m_visible; // внутреннее состояние
        int m_proportionalPos; // размер в % умноженных на 100.
        bool m_constructed; // закончено ли создание контрола
        FlatButton toggleButton; // кнопка переключения видимости панели
        bool m_mouseClicked; // кнопка нажата на контроле
        POINT m_mouseClickPoint; // координаты по которым нажата кнопка
        void selectToggleButtonLocation(LPRECT rect); //на входе clientrect - на выходе координаты кнопки
	class ItemInfo;	
	int PreviewAppsSize;
public:
	TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS>& getUserList() { return ctrlTransfers; }
        int getHeaderHeight() const { return m_headerHeight; }
        bool isVisible() const { return m_visible; }
        void setVisibility(bool value);
        void setProportionalPos(int value) { m_proportionalPos = value; }
        void setConstructed(bool value) { m_constructed = value; }
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM
	};

	enum {
		COLUMN_FIRST,
		COLUMN_USER = COLUMN_FIRST,
		COLUMN_HUB,
		COLUMN_STATUS,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_FILE,
		COLUMN_SIZE,
		COLUMN_PATH,
        COLUMN_LOCATION, // !SMT!-IP
		COLUMN_IP,
#ifdef PPA_INCLUDE_DNS
                COLUMN_DNS, // !SMT!-IP
#endif
		COLUMN_SHARE, //[+]PPA
		COLUMN_SLOTS, //[+]PPA
//[-]PPA		COLUMN_RATIO,
		COLUMN_LAST
	};

	enum {
		IMAGE_DOWNLOAD = 0,
		IMAGE_UPLOAD,
		IMAGE_SEGMENT
	};

	struct UpdateInfo;
	class ItemInfo : public UserInfoBase {
	public:
		typedef ItemInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::const_iterator Iter;

		ItemInfo::List subItems;

		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING,
			// special statuses
			TREE_DOWNLOAD,
			DOWNLOAD_STARTING,
			DOWNLOAD_FINISHED
		};

		ItemInfo(const UserPtr& u, bool aDownload);

		bool download;
		bool streamingDownload;
		bool transferFailed;
		bool collapsed;
		ItemInfo* main;
		Status status;
        int flagImage;
		int64_t pos;
		int64_t size;
		int64_t start;
		int64_t actual;
		int64_t speed;
		int64_t timeLeft;
		tstring Target;
		uint64_t fileBegin;
		bool multiSource;

		tstring columns[COLUMN_LAST];
		void update(const UpdateInfo& ui);

		void disconnect();
                void setPriorityPause();
		void removeAll();
		void deleteSelf() { delete this; }	

		double getRatio() const { return (pos > 0) ? (double)actual / (double)pos : 1.0; }

		const tstring& getText(int col) const {
			return columns[col];
		}

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col);

		int imageIndex() const { return !download ? IMAGE_UPLOAD : (!main ? IMAGE_DOWNLOAD : IMAGE_SEGMENT); }

		ItemInfo* createMainItem() {
	  		ItemInfo* h = new ItemInfo(user, true);
			h->Target = Target;
			h->columns[COLUMN_FILE] = Util::getFileName(h->Target);
			if (streamingDownload) {
				h->columns[COLUMN_FILE] += _T(" [video]");
			}
			h->columns[COLUMN_PATH] = Util::getFilePath(h->Target);
			h->columns[COLUMN_STATUS] = TSTRING(CONNECTING);
			h->columns[COLUMN_HUB] = _T("0 ") + TSTRING(NUMBER_OF_SEGMENTS);

			return h;
		}
		const tstring& getGroupingString() const { return Target; }
		void updateMainItem() {
			if(main->subItems.size() == 1) {
				ItemInfo* i = main->subItems.front();
				main->user = i->user;
                main->flagImage = i->flagImage;
				main->columns[COLUMN_USER] = Text::toT(main->user->getFirstNick());
				main->columns[COLUMN_HUB] = WinUtil::getHubNames(main->user).first;
				main->columns[COLUMN_IP] = i->columns[COLUMN_IP];
                                main->columns[COLUMN_LOCATION] = i->columns[COLUMN_LOCATION]; // !SMT!-IP
			} else {
				TCHAR buf[256];
				snwprintf(buf, sizeof(buf), _T("%d %s"), main->subItems.size(), CTSTRING(USERS));

				main->columns[COLUMN_USER] = buf;
				main->columns[COLUMN_IP] = Util::emptyStringT;
                main->columns[COLUMN_LOCATION] = Util::emptyStringT; // !SMT!-IP
			}
		}
	};

	struct UpdateInfo : public Task {
		enum {
			MASK_POS = 1 << 0,
			MASK_SIZE = 1 << 1,
			MASK_START = 1 << 2,
			MASK_ACTUAL = 1 << 3,
			MASK_SPEED = 1 << 4,
			MASK_FILE = 1 << 5,
			MASK_STATUS = 1 << 6,
			MASK_TIMELEFT = 1 << 7,
			MASK_IP = 1 << 8,
			MASK_STATUS_STRING = 1 << 9,
			MASK_COUNTRY = 1 << 10,
			MASK_SEGMENT = 1 << 11
		};

		bool operator==(const ItemInfo& ii) { return download == ii.download && user == ii.user; }

                UpdateInfo(const UserPtr& aUser, bool isDownload, bool isTransferFailed = false) : updateMask(0), user(aUser), download(isDownload), transferFailed(isTransferFailed), multiSource(false), fileList(false), flagImage(0) { }

		uint32_t updateMask;

		UserPtr user;
		bool download;
		bool transferFailed;
		bool fileList;
		tstring target;
		void setMultiSource(bool aSeg) { multiSource = aSeg; updateMask |= MASK_SEGMENT; }
		bool multiSource;
		void setStatus(ItemInfo::Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		ItemInfo::Status status;
		void setPos(int64_t aPos) { pos = aPos; updateMask |= MASK_POS; }
		int64_t pos;
		void setSize(int64_t aSize) { size = aSize; updateMask |= MASK_SIZE; }
		int64_t size;
		void setStart(int64_t aStart) { start = aStart; updateMask |= MASK_START; }
		int64_t start;
		void setActual(int64_t aActual) { actual = aActual; updateMask |= MASK_ACTUAL; }
		int64_t actual;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed;
		void setTimeLeft(int64_t aTimeLeft) { timeLeft = aTimeLeft; updateMask |= MASK_TIMELEFT; }
		int64_t timeLeft;
		void setStatusString(const tstring& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
		tstring statusString;
		void setFile(const tstring& aFile) { file = Util::getFileName(aFile); path = Util::getFilePath(aFile); target = aFile; updateMask|= MASK_FILE; }
		tstring file;
		tstring path;
        void setIP(const string& aIP); // !SMT!-IP
        string  ip;
#ifdef PPA_INCLUDE_DNS
                tstring dns;
#endif
                tstring location; // !SMT!-IP
                int flagImage; // !SMT!-IP
	};

        UserPtr selectedUser; // !SMT!-S

	void speak(uint8_t type, UpdateInfo* ui) { tasks.add(type, ui); PostMessage(WM_SPEAKER); }

	TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS> ctrlTransfers;
        class Selection : public TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS>::Selection {
        public:
          Selection(TransferView* view): TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS>::Selection(view->ctrlTransfers) {
          }
        };

        static ListColumn columns[];
        static ListColumnSettings listSettings;
	OMenu transferMenu;
	OMenu segmentedMenu;
	OMenu usercmdsMenu;
	OMenu previewMenu;
    OMenu copyMenu; // !SMT!-UI
	CImageListContainer arrows;
	CImageListContainer speedImages;
	CImageListContainer speedImagesBW;

	CIcon user;

	TaskQueue tasks;

	StringMap ucLineParams;

	ItemInfo::List transferItems;

	virtual void on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) throw();
	virtual void on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) throw();
	virtual void on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) throw();

	virtual void on(DownloadManagerListener::Complete, Download* aDownload, bool isTree) throw() { onTransferComplete(aDownload, false, aDownload->getTargetFileName(), isTree);}
	virtual void on(DownloadManagerListener::Failed, Download* aDownload, const string& aReason) throw();
	virtual void on(DownloadManagerListener::Starting, Download* aDownload) throw();
	virtual void on(DownloadManagerListener::Tick, const Download::List& aDownload) throw();
	virtual void on(DownloadManagerListener::Status, const UserPtr&, const string& aMessage) throw();

	virtual void on(UploadManagerListener::Starting, Upload* aUpload) throw();
	virtual void on(UploadManagerListener::Tick, const Upload::List& aUpload) throw();
	virtual void on(UploadManagerListener::Complete, Upload* aUpload) throw() { onTransferComplete(aUpload, true, aUpload->getSourceFile(), false); }

	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

	void onTransferComplete(Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree);

	void CollapseAll();
	void ExpandAll();
	bool mainItemTick(ItemInfo* i, bool);

	void setMainItem(ItemInfo* i) {
		if(i->main != NULL) {
			ItemInfo* h = i->main;		
			if(h->Target != i->Target) {
				ctrlTransfers.removeGroupedItem(i, false);
				ctrlTransfers.insertGroupedItem(i, false);
			}
		} else {
			i->main = ctrlTransfers.findMainItem(i->Target);
		}
	}
};

#endif // !defined(TRANSFER_VIEW_H)

/**
 * @file
 * $Id: TransferView.h,v 1.13 2008/03/24 17:38:57 alexey Exp $
 */
