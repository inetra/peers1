#pragma once
#include "EditIcon.h"
#include "EditWithIcon.h"
#include "FilterEdit.h"
#include "../client/StringTokenizer.h"
#include "../client/Client.h"
#include "../windows/TypedListViewCtrl.h"
#include "../windows/UserInfo.h"
#include "../peers/UserCopyInfoMenu.h"
#include "../peers/VirtualListView.h"
#include "../peers/BTreeMap.h"

class UserListControl;

class ATL_NO_VTABLE UserListControlListener {
public:
  virtual void onDoubleClickUser(UserListControl* /*control*/, UserInfo*) {}
  // этот метод использовался для того, чтобы родитель создавал меню.
  virtual bool buildUserListMenu(UserPtr& /*aUser*/, OMenu* /*menu*/) { return true; }
  // этот метод использовался для того, чтобы родитель почистил созданное им меню.
  virtual void cleanUserListMenu(OMenu* /*menu*/) {}
  virtual void userListMessage(const tstring& /*line*/) {}
  virtual UserInfo::UserMap& getUserMap() = 0;
  virtual Client* getClient() = 0;
};

class UserFilter {
public:
  virtual bool match(const UserInfo* /*ui*/) const { return true; }
  virtual ~UserFilter() {}
};

class UserListContainer {
private:
	typedef vector<const UserInfo*> UserList;
	UserList m_userList;
	BTreeMap<const UserInfo*,size_t> m_userMap;
	auto_ptr<UserInfoComparator> m_comparator;

	size_t getInsertPos(const UserInfo* a) {
		size_t len = m_userList.size();
		if (m_comparator->isEmpty() || len == 0) {
			return len;
		}
		return getInsertPos(a, 0, len);
	}

public:
	static const size_t npos = ~(size_t)0;

	UserListContainer(): m_comparator(new UserInfoNickComparator()) {
	}

	void push_back(const UserInfo* ui) {
		m_userList.push_back(ui);
		m_userMap.put(ui, m_userList.size() - 1);
	}

	size_t add(const UserInfo* ui) {
		size_t index = getInsertPos(ui);
		dcassert(index <= m_userList.size());
		m_userList.insert(m_userList.begin() + index, ui);
		for (BTreeMapIterator<const UserInfo*,size_t> i = m_userMap.iterator(); i; ++i) {
			if (i->value >= index) {
				++i.getValue();
			}
			dcassert(i.getValue() < m_userList.size());
		}
		m_userMap.put(ui, index);
		return index;
	}

	size_t getInsertPos(const UserInfo* a, size_t low, size_t len) {
		while (len > 0) {
			const size_t half = len / 2;
			const UserInfo *b = m_userList[low + half];
			const int comp = m_comparator->compare(a, b);

			if (comp == 0) {
				return low + half;
			} else if (comp > 0) {
				low += half + 1;
				len -= half + 1;
			} else {
				dcassert(comp < 0);
				len = half;
			}
		}
		return low;
	}

	size_t indexOf(const UserInfo* ui) const {
		size_t index;
		if (m_userMap.find(ui, index)) {
			return index;
		}
		else {
			return npos;
		}
	}

	size_t remove(const UserInfo* ui) {
		size_t index;
		if (m_userMap.find(ui, index)) {
			m_userMap.remove(ui);
			m_userList.erase(m_userList.begin() + index);
			for (BTreeMapIterator<const UserInfo*,size_t> i = m_userMap.iterator(); i; ++i) {
				if (i->value >= index) {
					--i.getValue();
				}
				dcassert(i.getValue() < m_userList.size());
			}
			return index;
		}
		return npos;
	}

	void clear() {
		m_userList.clear();
		m_userMap.clear();
	}

	const UserInfo* operator [](size_t index) const {
		if (index < m_userList.size()) {
			return m_userList[index];
		}
		else {
			return NULL;
		}
	}

	size_t size() const {
		return m_userList.size();
	}

	struct CompareOperation {
		UserInfoComparator* m_comparator;
		CompareOperation(UserInfoComparator* comparator): m_comparator(comparator) { }
		bool operator() (const UserInfo* a, const UserInfo* b) {
			return m_comparator->compare(a, b) < 0;
		}
	};

	void sort() {
		std::sort(m_userList.begin(), m_userList.end(), CompareOperation(m_comparator.get()));
		// m_userMap.clear();
		size_t index = 0;
		for (UserList::iterator i = m_userList.begin(); i != m_userList.end(); ++i) {
			const UserInfo* ui = *i;
			m_userMap.put(ui, index++);
		}
	}

	enum ValidationStatus {
		vsOK,
		vsPrevGreater,
		vsNextSmaller
	};

	ValidationStatus isOrderValid(size_t pos, UserInfo* ui) {
		dcassert(ui == m_userList[pos]);
		if (pos > 0) {
			const UserInfo* prev = m_userList[pos - 1];
			if (m_comparator->compare(prev, ui) > 0) {
				return vsPrevGreater;
			}
		}
		if (pos < m_userList.size() - 1) {
			const UserInfo* next = m_userList[pos + 1];
			if (m_comparator->compare(ui, next) > 0) {
				return vsNextSmaller;
			}
		}
		return vsOK;
	}

	void setComparator(UserInfoComparator* comparator) {
		m_comparator.reset(comparator);
	}

	void move(size_t pos, size_t newPos) {
		size_t low;
		size_t high;
		if (pos < newPos) {
			low = pos;
			high = newPos;
			m_userList.insert(m_userList.begin() + newPos, m_userList[pos]);
			m_userList.erase(m_userList.begin() + pos);
		}
		else {
			low = newPos;
			high = pos;
			const UserInfo* ui = m_userList[pos];
			m_userList.erase(m_userList.begin() + pos);
			m_userList.insert(m_userList.begin() + newPos, ui);
		}
		for (size_t i = low; i <= high; ++i) {
			m_userMap.put(m_userList[i], i);
		}
	}

};

class UserListControl : 
  public CWindowImpl<UserListControl>,
  public UserInfoBaseHandler<UserListControl>,
  private LV::ListViewListener<UserInfo>,
  UserInfoGetter
{
private:
  static const LV::Column UserListControl::defaultColumns[COLUMN_LAST];
  UserListControlListener* m_listener;

  bool isFilterActive() { return !m_filter.empty(); }
  tstring m_filter;
  bool sorted;

  vector<const UserInfo*> selectedObjects;
  set<size_t> selectedIndexes;
  const UserInfo* focused;
  bool invalidateCount;
  size_t invalidateBegin;
  size_t invalidateEnd; //after

  CFilterEdit ctrlFilter;
  LV::VirtualListView<UserInfo,const UserInfo*> ctrlUsers;
  UserListContainer m_users;
  /* высота заголовка */
  int m_headerHeight;
  UserCopyInfoMenu m_copyMenu;

  UserFilter* getCurrentFilter();

  void onCustomDrawUserItem(const UserInfo* ui, LPNMLVCUSTOMDRAW cd);

  void resortList() {
	  dcdebug("resortList\n");
	  m_users.sort();
	  sorted = true;
  }

  void saveSelection() {
	  selectedObjects.clear();
	  selectedIndexes.clear();
	  focused = NULL;
	  int i = -1;
	  while ((i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		  selectedIndexes.insert(i);
		  selectedObjects.push_back(getItem(i));
	  }
	  i = ctrlUsers.GetNextItem(-1, LVNI_FOCUSED);
	  if (i != -1) {
		  focused = getItem(i);
	  }
  }

  void countChanged() {
	  invalidateCount = true;
  }

  void redrawItems(size_t first, size_t last) {
	  if (invalidateBegin == 0 && invalidateEnd == 0) {
		  invalidateBegin = first;
		  invalidateEnd = last;
	  }
	  else {
		  if (first < invalidateBegin) {
			  invalidateBegin = first;
		  }
		  if (last > invalidateEnd) {
			  invalidateEnd = last;
		  }
	  }
  }

  // UserInfoGetter
  virtual UserInfo* getSelectedUser() { 
    if (m_listener && aSelectedUser) {
      return m_listener->getUserMap().findUser(aSelectedUser);
    }
    return NULL;
  }

  // ListViewListener<UserInfo>
  virtual const UserInfo* getItem(int index) {
	  return m_users[index];
  }

  virtual tstring getText(const UserInfo* item, int columnIndex) const {
	  return item->getText(columnIndex);
  }

  virtual int getItemImage(const UserInfo* item) const {
	  return item->imageIndex();
  }

public:

  UserListControl(UserListControlListener* listener);

  virtual ~UserListControl() {}

  void initColumns(const tstring& server);

  void addItem(UserInfo* ui);
  void updateItem(UserInfo* ui);
  void deleteItem(UserInfo* ui);

  void reloadUserList();

  void beginUpdate();
  void endUpdate();

  void clear() {
	  m_users.clear();
	  ctrlUsers.SetItemCountEx(0, 0);
  }

  void locateInList(UserInfo* ui);
  void UserListControl::locateInList(const tstring& nick);

  FavoriteHubEntry* saveColumns(const tstring& server);

  /* копирует выбранных пользователей в переданный список */
  void getSelectedUsers(vector<const UserInfo*>& users) const {
	  ctrlUsers.getSelection(users);
  }

  void SetFocus() {
    ctrlUsers.SetFocus();
  }

  void unselectAll() {
    int y = ctrlUsers.GetItemCount();
    for (int x = 0; x < y; ++x) {
      ctrlUsers.SetItemState(x, 0, LVNI_FOCUSED | LVNI_SELECTED);
    }
  }

  /* ищет пользователея по префиксу, возвращает найденное имя или пустую строку если не найдено */
  tstring autoCompleteUserNick(const tstring& prefix);

  int GetSelectedCount() const {
    return ctrlUsers.GetSelectedCount();
  }

  int getItemCount() const {
    return ctrlUsers.GetItemCount();
  }

  bool isCountChanged() const {
	  return invalidateCount;
  }

  void updateColors();

  UserPtr& getSelectedUserPtr() {
    return aSelectedUser;
  }

  void setSelectedUserPtr(UserPtr& user) {
    aSelectedUser = user;
  }

  HWND Create(HWND hWndParent) {
    return CWindowImpl<UserListControl>::Create(hWndParent, NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  }

  int decodeDoubleClickAction(int action) const;

  /* возвращает код команды, котораю должна выполняться по двойному клику на пользователе в списке */
  virtual int getDoubleClickAction() const;

  // required by UserInfoBaseHandler<>
  LV::VirtualListView<UserInfo,const UserInfo*>& getUserList() { return ctrlUsers; }

  BEGIN_MSG_MAP(UserListControl)
    MESSAGE_HANDLER(WM_CREATE, onCreate)
    MESSAGE_HANDLER(WM_SIZE, onSize)
    MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd)
    MESSAGE_HANDLER(WM_PAINT, onPaint)

    NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClickUsers)
    NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDownUsers)
    NOTIFY_HANDLER(IDC_USERS, NM_RETURN, onEnterUsers)
    NOTIFY_HANDLER(IDC_USERS, NM_CUSTOMDRAW, onCustomDraw)
    //NOTIFY_HANDLER(IDC_USERS, LVN_ITEMCHANGED, onItemChanged)
    NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
    NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, onColumnClick)
    NOTIFY_HANDLER(IDC_USERS, LVN_GETINFOTIP, ctrlUsers.onInfoTip)
    
    COMMAND_HANDLER(IDC_USER_LIST_FILTER, EN_CHANGE, onFilterChange)

    MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
    MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
    MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

    CHAIN_MSG_MAP_MEMBER(m_copyMenu)
    CHAIN_COMMANDS(UserInfoBaseHandler<UserListControl>)

  END_MSG_MAP()

  LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

  LRESULT onDoubleClickUsers(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
  LRESULT onEnterUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
  LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

  LRESULT onKeyDownUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    NMLVKEYDOWN* l = (NMLVKEYDOWN*)pnmh;
    if (l->wVKey == VK_TAB) {
      //TODO: onTab();
    }
    return 0;
  }

  LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
    //TODO: updateStatusBar();
    return 0;
  }

  LRESULT onFilterChange(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
  LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

};
