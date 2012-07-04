#pragma once
#include "../client/QueueItem.h"
#include "TransparentBitmap.h"

class PrioritySlider : public CWindowImpl<PrioritySlider> {
private:
  static QueueItem::Priority priorities[];
  static WORD priorityCommandCodes[];
  int m_imageIndex;
  TransparentBitmap m_images[4];
  enum {
    PREFERRED_WIDTH = 125,
    PREFERRED_HEIGHT = 29
  };
public:
  PrioritySlider();

  int getPreferredWidth() const {
    return PREFERRED_WIDTH;
  }

  int getPreferredHeight() const {
    return PREFERRED_HEIGHT;
  }

  void setPriority(QueueItem::Priority priority);

  BEGIN_MSG_MAP(PrioritySlider)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
    MESSAGE_HANDLER(WM_ENABLE, onEnable);
    MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown);
  END_MSG_MAP();

  LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onEnable(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    Invalidate();
    return 0;
  }
  LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

};
