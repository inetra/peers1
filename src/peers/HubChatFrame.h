#pragma once
#include "../windows/HubFrame.h"
#include "ChatControl.h"
#include "UserListControl.h"

class ChatUserListControl : public UserListControl {
public:
	ChatUserListControl(UserListControlListener* listener): UserListControl(listener) {
	}

	virtual int getDoubleClickAction() const {
		return decodeDoubleClickAction(SETTING(USERLIST_CHAT_DBLCLICK));
	}
};

class HubChatFrame : 
  public MDITabChildWindowImpl<HubChatFrame, IDI_CHAT_ONLINE, IDI_CHAT_OFFLINE>,
  public WindowSortOrder,
  public CSplitterImpl<HubChatFrame>,
  private SettingsManagerListener,
  private ChatControlListener
{
protected:
  friend class HubFrame;
  typedef CSplitterImpl<HubChatFrame> splitBase;
  typedef MDITabChildWindowImpl<HubChatFrame, IDI_CHAT_ONLINE, IDI_CHAT_OFFLINE> baseClass;
public:
  DECLARE_FRAME_WND_CLASS_EX(_T("HubChatFrame"), IDI_CHAT_ONLINE, 0, COLOR_3DFACE);

  BEGIN_MSG_MAP(HubChatFrame)
    MESSAGE_HANDLER(WM_CREATE, OnCreate);
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, OnPaint);
    MESSAGE_HANDLER(WM_CLOSE, OnClose);
    MESSAGE_HANDLER(WPM_AFTER_CREATE, onAfterCreate);
    CHAIN_MSG_MAP(baseClass);
    CHAIN_MSG_MAP(splitBase);
  END_MSG_MAP();

  LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
  LRESULT OnEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
  virtual void onActivate();
  LRESULT onAfterCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

  HubChatFrame(HubFrame *hubFrame, ChatControlListener* chatListener, UserListControlListener* userListListener);

  void UpdateLayout(BOOL bResizeBars = TRUE);

  // ChatControlListener
  virtual Client* getClient() { return m_chatListener->getClient(); }

private:
  HubFrame *m_hubFrame;
  ChatControlListener* m_chatListener;
  ChatControl m_chatControl;
  ChatUserListControl m_userListControl;

  virtual bool isLargeIcon() const { return true; }
  
  // SettingsManagerListener
  virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

  // ChatControlListener
  virtual bool chatExecuteCommand(ChatCommandContext* context) { return m_chatListener->chatExecuteCommand(context); }
  virtual void chatSendMessage(const tstring& s) { m_chatListener->chatSendMessage(s); }
  virtual UserInfo* findUser(const tstring& nick) { return m_chatListener->findUser(nick); }
  virtual void onUserDblClick(UserInfo* ui, WPARAM wParam) { m_chatListener->onUserDblClick(ui, wParam); }
  virtual bool onUserContextMenu(UserInfo* ui, CPoint pt) { return m_chatListener->onUserContextMenu(ui, pt); }
  virtual void autoCompleteFaliure() { m_userListControl.SetFocus(); }
  virtual void autoCompleteBegin() { m_userListControl.unselectAll(); }
  virtual tstring autoCompleteUserNick(const tstring& prefix) { return m_userListControl.autoCompleteUserNick(prefix); }

  // WindowSortOrder
  virtual WindowSortOrders getSortOrder();
};
