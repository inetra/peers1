#pragma once
#include "../client/Client.h"
#include "../windows/OMenu.h"
#include "../windows/ChatCtrl.h"
#include "ChatCommand.h"
#include "TransparentBitmap.h"

class ATL_NO_VTABLE ChatControlListener {
public:
  /* возвращает конект к хабу */
  virtual Client* getClient() = 0;
  /* обработка команды */
  virtual bool chatExecuteCommand(ChatCommandContext* context) = 0;
  /* обработка сообщения */
  virtual void chatSendMessage(const tstring& s) = 0;
  /* возвращает пользователя по имени */
  virtual UserInfo* findUser(const tstring& nick) = 0;
  /* сообщает о двойном клике по пользователю */
  virtual void onUserDblClick(UserInfo* ui, WPARAM wParam) = 0;
  /* контекстное меню чата. пользователь может быть NULL */
  virtual bool onUserContextMenu(UserInfo* ui, CPoint pt) = 0;
  /* неудачная попытка начать автозавершение имени пользователя с пустой строкой */
  virtual void autoCompleteFaliure() = 0;
  /* начинает сеанс автозавершения имени пользователя - снимает выделение со всех элементов списка пользователей */
  virtual void autoCompleteBegin() = 0;
  /* автозавершение имени пользователя */
  virtual tstring autoCompleteUserNick(const tstring& prefix) = 0;
};

class ChatControl : public CWindowImpl<ChatControl> {
private:
  enum {
    mapView = 1,
    mapMessage = 2,
    mapEmoticons = 3
  };
  ChatControlListener* m_listener;
  CEmotionMenu emoMenu;
  CButton ctrlEmoticons;
  CButton ctrlSend;
  ChatCtrl ctrlClient;
  CEdit ctrlMessage;
  CContainedWindow containerEmoticons;
  CContainedWindow containerMessage;
  CContainedWindow containerView;

  TStringList prevCommands;
  tstring currentCommand;
  TStringList::size_type curCommandPosition;		//can't use an iterator because StringList is a vector, and vector iterators become invalid after resizing

  tstring complete;
  void onTab();

  tstring sSelectedURL;

  void addCommandToHistory(const tstring& command);
  void onSend();

  bool timeStamps;
  int m_headerHeight;
  TransparentBitmap m_headerIcon;

  tstring findTextPopup();
  tstring currentNeedle;		// search in chat window
  int currentNeedlePos;		// search in chat window
  void findText(tstring const& needle) throw();

  void processCommand(ChatCommandContext& context);

  typedef void (ChatControl::*ChatCommandMethod)(ChatCommandContext*);
  typedef map<string, ChatCommandMethod> ChatCommandMap;
  static ChatCommandMap m_commands;
  static void initCommandMap();
  void commandClear(ChatCommandContext* context);
  void commandFind(ChatCommandContext* context);
  void commandToggleTimestamps(ChatCommandContext*);

  bool processHotKey(WPARAM wParam);

public:

  ChatControl(ChatControlListener* listener);
  virtual ~ChatControl() {}

  HWND Create(HWND hWndParent) {
    return CWindowImpl<ChatControl>::Create(hWndParent, NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  }

  void SetFocus() {
    ctrlMessage.SetFocus();
  }

  void setMessageText(const TCHAR* text);
  void addUserToMessageText(const tstring& sUser);
  void addLine(const Identity& i, const string& nick, const string& extra, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/);
  void addSystemMessage(const tstring& aLine);
  bool isTimeStamps() const { return timeStamps; }
  bool isSelectedURL() const { return !sSelectedURL.empty(); }
  bool isAutoScroll() const { return ctrlClient.GetAutoScroll(); }
  void updateColors();

  BEGIN_MSG_MAP(ChatControl)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_SIZE, onSize);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, onSizeMove);

    MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
    MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
    MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

    COMMAND_HANDLER(IDC_CHAT_SEND, BN_CLICKED, onSend);
    COMMAND_HANDLER(IDC_EMOT, BN_CLICKED, onEmoticons);
    COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + emoMenu.m_menuItems, onEmoPackChange);

    COMMAND_ID_HANDLER(ID_EDIT_COPY, onEditCopy);
    COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, onEditSelectAll);
    COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll);
    COMMAND_ID_HANDLER(IDC_COPY_ACTUAL_LINE, onCopyActualLine);
    COMMAND_ID_HANDLER(IDC_AUTOSCROLL_CHAT, onAutoScrollChat);
    COMMAND_ID_HANDLER(IDC_COPY_URL, onCopyURL);

    NOTIFY_HANDLER(IDC_CLIENT, EN_LINK, onClientEnLink);

  ALT_MSG_MAP(mapView)
    MESSAGE_HANDLER(WM_CONTEXTMENU, onViewContextMenu);
    MESSAGE_HANDLER(WM_MOUSEMOVE, onViewMouseMove);
    MESSAGE_HANDLER(WM_RBUTTONUP, onViewMouseMove);
	MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onViewLButtonDblClick);
    MESSAGE_HANDLER(WM_KEYDOWN, onViewKeyDown);

  ALT_MSG_MAP(mapMessage)
    MESSAGE_HANDLER(WM_KEYDOWN, onMessageKeyDown)
    MESSAGE_HANDLER(WM_CHAR, onMessageChar)

  ALT_MSG_MAP(mapEmoticons)
    MESSAGE_HANDLER(WM_CONTEXTMENU, onEmoticonsContextMenu)

  END_MSG_MAP();

  LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onSizeMove(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled); 

  LRESULT onSend(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
  LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  LRESULT onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onAutoScrollChat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  LRESULT onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

  LRESULT onEmoticonsContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

  LRESULT onMessageKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onMessageChar(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

  LRESULT onViewContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onViewMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onViewLButtonDblClick(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT onViewKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

};
