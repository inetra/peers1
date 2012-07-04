#include "stdafx.h"
#include "HubMessageControl.h"
#include "PeersVersion.h"

#define ICON_WIDTH  32
#define ICON_HEIGHT 32

HubMessageControl::HubMessageControl():
m_wheelAccumulator(0),
maxMessageCount(512),
scrollTop(0),
messageHeightSum(0),
m_lineHeight(16),
m_topMargin(4),
m_leftMargin(ICON_WIDTH + 16),
m_rightMargin(2),
m_bottomMargin(4)
{
}

HWND HubMessageControl::Create(HWND hWndParent) {
  return CWindowImpl<HubMessageControl>::Create(hWndParent, NULL, NULL, WS_VISIBLE | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
}

int HubMessageControl::getScrollPage(int lines) const { 
  return max(ICON_HEIGHT, m_lineHeight * lines); 
}

int HubMessageControl::getScrollPage() const {
  return getScrollPage(m_visibleLines);
}

bool HubMessageControl::isScrollBarVisible() {
  SCROLLINFO si;
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE | SIF_PAGE;
  return GetScrollInfo(SB_VERT, &si) && si.nMin - 1 + (int) si.nPage <= si.nMax;
}

class MessageHeightMeasurer {
private:
  CClientDC dc;
  CRect clientRect;
  const HFONT oldFont;
public:
  MessageHeightMeasurer(HubMessageControl* owner): dc(owner->m_hWnd), oldFont(dc.SelectFont(WinUtil::systemFont)) {
    owner->GetClientRect(clientRect);
    if (!owner->isScrollBarVisible()) {
      clientRect.right -= GetSystemMetrics(SM_CXVSCROLL);
    }
    clientRect.left += owner->m_leftMargin;
    clientRect.right -= owner->m_rightMargin;
  }
  ~MessageHeightMeasurer() {
    dc.SelectFont(oldFont);
  }
  int measureHeight(const tstring& s) {
    CRect r(clientRect);
    dc.DrawText(s.c_str(), s.length(), r, DT_CALCRECT | DT_WORDBREAK);
    return r.Height();
  }
};

void HubMessageControl::addMessage(const tstring& hubName, const tstring& author, const tstring& message) {
  tstring text = hubName + _T(": ");
  if (!author.empty()) {
    text += author + _T(": ");
  }
  text += message;
  const int msgHeight = MessageHeightMeasurer(this).measureHeight(text);
  messages.push_back(HubMessage());
  if (messages.size() > maxMessageCount) {
    messageHeightSum -= messages.front().height;
    messages.erase(messages.begin());
  }
  HubMessage& last = messages.back();
  last.time = Text::toT(Util::getShortTimeString());
  last.text = text;
  last.height = msgHeight;
  messageHeightSum += last.height;
  scrollTop = max(messageHeightSum - getScrollPage(), 0);
  SCROLLINFO si;
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
  si.nMin = 0;
  si.nMax = messageHeightSum - 1;
  si.nPage = min(messageHeightSum, getScrollPage());
  si.nPos = scrollTop;
  SetScrollInfo(SB_VERT, &si);
  Invalidate();
}

LRESULT HubMessageControl::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_lineHeight = measureFontHeight();
  return 0;
}

LRESULT HubMessageControl::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  m_visibleLines = max((int) PREFERRED_LINE_COUNT, (HIWORD(lParam) - m_topMargin - m_bottomMargin) / m_lineHeight);
  if (!messages.empty()) {
    const double oldScrollRatio = (double) scrollTop / messageHeightSum;
    //dcdebug("scrollTop=%d oldScrollRatio=%d \n", scrollTop, oldScrollRatio);
    MessageHeightMeasurer measurer(this);
    messageHeightSum = 0;
    for (HubMessageIterator i = messages.begin(); i != messages.end(); ++i) {
      HubMessage& msg = *i;
      msg.height = measurer.measureHeight(msg.text);
      messageHeightSum += msg.height;
    }
    scrollTop = (int) floor(oldScrollRatio * messageHeightSum + 0.5);
    if (scrollTop < 0) {
      scrollTop = 0;
    }
    else {
      scrollTop = min(scrollTop, max(0, messageHeightSum - getScrollPage()));
    }
    //dcdebug("NEW scrollTop=%d \n", scrollTop);
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    si.nMin = 0;
    si.nMax = messageHeightSum - 1;
    si.nPage = min(messageHeightSum, getScrollPage());
    si.nPos = scrollTop;
    SetScrollInfo(SB_VERT, &si);
  }
  return 0;
}

int HubMessageControl::measureFontHeight() {
  CClientDC dc(m_hWnd);
  const HFONT oldFont = dc.SelectFont(WinUtil::systemFont);
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  dc.SelectFont(oldFont);
  return tm.tmHeight;
}

LRESULT HubMessageControl::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_INFOBK));
  CIcon helpIcon;
  helpIcon.LoadIcon(IDI_SETTINGS_HELP, ICON_WIDTH, ICON_HEIGHT);
  if (helpIcon) {
    helpIcon.DrawIconEx(dc, (m_leftMargin - ICON_WIDTH) / 2, (r.Height() - ICON_HEIGHT) / 2, ICON_WIDTH, ICON_HEIGHT);
  }
  dc.SetBkColor(GetSysColor(COLOR_INFOBK));
  dc.SetTextColor(GetSysColor(COLOR_INFOTEXT));
  const HFONT oldFont = dc.SelectFont(WinUtil::systemFont);
  r.top += m_topMargin;
  r.left += m_leftMargin;
  r.right -= m_rightMargin;
  r.bottom -= m_bottomMargin;
  const int screenTop = r.top;
  const int screenBottom = r.bottom;
  r.top -= scrollTop;
  for (HubMessageIterator i = messages.begin(); i != messages.end(); ++i) {
    const HubMessage& msg = *i;
    r.bottom = r.top + msg.height;
    //dcdebug("scrollTop=%d r.top=%d r.bottom=%d screenTop=%d screenBottom=%d \n", scrollTop, r.top, r.bottom, screenTop, screenBottom);
    if (r.bottom > screenTop) {
      tstring tmp = msg.time + _T(" ") + msg.text;
      dc.DrawText(tmp.c_str(), tmp.length(), r, DT_WORDBREAK);
    }
    if (r.bottom >= screenBottom) break;
    r.top = r.bottom;
  }
  dc.SelectFont(oldFont);
  if (GetFocus() == m_hWnd) {
    drawFocusRect(dc);
  }
  return 0;
}

void HubMessageControl::drawFocusRect(HDC dc) {
  CRect r;
  GetClientRect(r);
  r.top += min(1, m_topMargin);
  r.left += min(1, m_leftMargin);
  r.right -= min(1, m_rightMargin);
  r.bottom -= min(1, m_bottomMargin);
  ::DrawFocusRect(dc, r);
}

LRESULT HubMessageControl::onVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  int newPos = scrollTop;
  switch (LOWORD(wParam)) 
  {
  case SB_TOP:
    newPos = 0;
    break;
  case SB_BOTTOM:
    newPos = messageHeightSum - getScrollPage();
    break;
  case SB_LINEDOWN:
    newPos += m_lineHeight;
    break;
  case SB_LINEUP:
    newPos -= m_lineHeight;
    break;
  case SB_PAGEDOWN:
    newPos += getScrollPage();
    break;
  case SB_PAGEUP:
    newPos -= getScrollPage();
    break;
  case SB_THUMBPOSITION:
    {
      SCROLLINFO si;
      si.cbSize = sizeof(SCROLLINFO);
      si.fMask = SIF_TRACKPOS;
      if (GetScrollInfo(SB_VERT, &si)) {
        newPos = si.nTrackPos;
      }
    }
    break;
  case SB_THUMBTRACK:
    {
      SCROLLINFO si;
      si.cbSize = sizeof(SCROLLINFO);
      si.fMask = SIF_TRACKPOS;
      if (GetScrollInfo(SB_VERT, &si)) {
        newPos = si.nTrackPos;
      }
    }
  }
  if (newPos < 0) {
    newPos = 0;
  }
  else {
    const int maxPos = max(0, messageHeightSum - getScrollPage());
    if (newPos > maxPos) {
      newPos = maxPos;
    }
  }
  if (newPos != scrollTop) {
    dcdebug("newPos=%d\n", newPos);
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    si.nPos = newPos;
    SetScrollInfo(SB_VERT, &si);
    scrollTop = newPos;
    Invalidate();
  }
  return 0;
}

int HubMessageControl::getHeight() const {
  CRect r;
  GetClientRect(r);
  return r.Height();
}

LRESULT HubMessageControl::onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
  const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
  const int xOffset = (m_leftMargin - ICON_WIDTH) / 2, yOffset = (getHeight() - ICON_HEIGHT) / 2;
  if (x >= xOffset && x < xOffset + ICON_WIDTH && y >= yOffset && y < yOffset + ICON_HEIGHT) {
    SetCursor(::LoadCursor(NULL, IDC_HAND));
  }
  else {
    bHandled = FALSE;
  }
  return 0;
}

LRESULT HubMessageControl::onMouseDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  SetFocus();
  const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
  const int xOffset = (m_leftMargin - ICON_WIDTH) / 2, yOffset = (getHeight() - ICON_HEIGHT) / 2;
  if (x >= xOffset && x < xOffset + ICON_WIDTH && y >= yOffset && y < yOffset + ICON_HEIGHT) {
    WinUtil::openLink(HOMEPAGE);
  }
  return 0;
}

LRESULT HubMessageControl::onMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_wheelAccumulator += GET_WHEEL_DELTA_WPARAM(wParam);
  while (abs(m_wheelAccumulator) >= WHEEL_DELTA) {
    const bool isNeg = m_wheelAccumulator < 0;
    m_wheelAccumulator = abs(m_wheelAccumulator) - WHEEL_DELTA;
    if (isNeg) {
      if (m_wheelAccumulator != 0) m_wheelAccumulator = -m_wheelAccumulator;
      SendMessage(WM_VSCROLL, SB_LINEDOWN);
    }
    else {
      SendMessage(WM_VSCROLL, SB_LINEUP);
    }
  }
  return 0;
}

LRESULT HubMessageControl::onChangeFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (GetUpdateRect(NULL, FALSE)) {
    Invalidate();
  }
  else {
    dcdebug("call drawFocusRect in onChangeFocus\n");
    CClientDC dc(m_hWnd);
    drawFocusRect(dc);
  }
  return 0;
}
