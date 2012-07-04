#pragma once
#include "StaticAccessSingleton.h"

class HubMessageControl : public CWindowImpl<HubMessageControl>, public StaticAccessSingleton<HubMessageControl> {
private:
  friend class MessageHeightMeasurer;

  struct HubMessage {
    tstring time;
    tstring text;
    int height;
  };
  typedef vector<HubMessage> HubMessageList;
  typedef HubMessageList::iterator HubMessageIterator;
  typedef HubMessageList::reverse_iterator HubMessageRIterator;

  enum {
    PREFERRED_LINE_COUNT = 3
  };

  HubMessageList messages;
  size_t maxMessageCount;

  int scrollTop;
  int messageHeightSum;
  int m_lineHeight;
  int m_leftMargin;
  int m_rightMargin;
  int m_topMargin;
  int m_bottomMargin;
  int m_wheelAccumulator;
  int m_visibleLines;

  bool isScrollBarVisible();
  int measureFontHeight();
  int getHeight() const;
  void drawFocusRect(HDC dc);

public:
  HubMessageControl();
  virtual ~HubMessageControl() { }

  HWND Create(HWND hWndParent);

  int getPreferredHeight() const { return getScrollPage(PREFERRED_LINE_COUNT) + m_topMargin + m_bottomMargin; }
  int getScrollPage() const;
  int getScrollPage(int lines) const;

  void addMessage(const tstring& hubName, const tstring& author, const tstring& message);

  BEGIN_MSG_MAP(HubMessageControl)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_SIZE, onSize);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
    MESSAGE_HANDLER(WM_VSCROLL, onVScroll);
    MESSAGE_HANDLER(WM_MOUSEMOVE, onMouseMove);
    MESSAGE_HANDLER(WM_LBUTTONDOWN, onMouseDown);
    MESSAGE_HANDLER(WM_MOUSEWHEEL, onMouseWheel);
    MESSAGE_HANDLER(WM_SETFOCUS, onChangeFocus);
    MESSAGE_HANDLER(WM_KILLFOCUS, onChangeFocus);
  END_MSG_MAP();

  LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onVScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onMouseDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onMouseWheel(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onChangeFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

};
