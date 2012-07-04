#include "stdafx.h"
#include "PrioritySlider.h"

QueueItem::Priority PrioritySlider::priorities[] = {
  QueueItem::PAUSED,
  QueueItem::LOW,
  QueueItem::NORMAL,
  QueueItem::HIGH
};

WORD PrioritySlider::priorityCommandCodes[] = {
  IDC_PRIORITY_PAUSED,
  IDC_PRIORITY_LOW,
  IDC_PRIORITY_NORMAL,
  IDC_PRIORITY_HIGH
};

static ATL::_U_STRINGorID resourceIds[4] = {
  IDB_DOWNLOAD_PRIORITY_PAUSE,
  IDB_DOWNLOAD_PRIORITY_LOW,
  IDB_DOWNLOAD_PRIORITY_NORMAL,
  IDB_DOWNLOAD_PRIORITY_HIGH
};

PrioritySlider::PrioritySlider(): m_imageIndex(0) {
}

LRESULT PrioritySlider::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  for (int i = 0; i < COUNTOF(m_images); ++i) {
    m_images[i].init(resourceIds[i], RGB(255,0,255));
    dcassert(m_images[i].getWidth() == PREFERRED_WIDTH);
    dcassert(m_images[i].getHeight() == PREFERRED_HEIGHT);
  }
  return 0;
}

LRESULT PrioritySlider::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_BTNFACE));
  if (IsWindowEnabled()) {
    m_images[m_imageIndex].draw(dc, 0, 0);
  }
  else {
    CDC bitmapDC;
    bitmapDC.CreateCompatibleDC(dc);
    CBitmap tempBitmap;
    tempBitmap.CreateCompatibleBitmap(dc, r.Width(), r.Height());
    const HBITMAP oldBitmap = bitmapDC.SelectBitmap(tempBitmap);
    bitmapDC.FillRect(r, (HBRUSH) GetStockObject(WHITE_BRUSH));
    m_images[COUNTOF(m_images) - 1].draw(bitmapDC, 0, 0);
    dc.DitherBlt(0, 0, PREFERRED_WIDTH, PREFERRED_HEIGHT, bitmapDC, NULL, 0, 0);
    bitmapDC.SelectBitmap(oldBitmap);
  }
  return 0;
}

LRESULT PrioritySlider::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
  CRect r;
  GetClientRect(r);
  if (x >= r.left && x < r.right && y >= r.top && y < r.bottom) {
    const int interval = r.Width() / (COUNTOF(m_images) - 1);
    const int newIndex = (x - r.left + interval / 2) / interval;
    if (newIndex != m_imageIndex) {
      m_imageIndex = newIndex;
      Invalidate();
      ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(priorityCommandCodes[newIndex],0), 0);
    }
  }
  return 0;
}

void PrioritySlider::setPriority(QueueItem::Priority priority) {
  int newIndex;
  if (priority >= QueueItem::HIGH) {
    newIndex = 3;
  }
  else if (priority == QueueItem::NORMAL) {
    newIndex = 2;
  }
  else if (priority > QueueItem::PAUSED) {
    newIndex = 1;
  }
  else {
    newIndex = 0;
  }
  if (newIndex != m_imageIndex) {
    m_imageIndex = newIndex;
    Invalidate();
  }
}
