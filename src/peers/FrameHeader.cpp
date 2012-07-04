#include "stdafx.h"
#include "FrameHeader.h"
#include "ControlAdjuster.h"

FrameHeader::FrameHeader(): captionFont(CaptionFont::BOLD), m_headerHeight(32), m_leftMargin(8) {
}

LRESULT FrameHeader::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_headerHeight = ControlAdjuster::adjustHeaderHeight(m_hWnd);
  return 0;
}

LRESULT FrameHeader::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_3DFACE));
  dc.SetBkColor(GetSysColor(COLOR_3DFACE));    
  draw((HDC) dc, m_leftMargin, max(0, r.Height() - m_textHeight) / 2);
  return 0;
}

void FrameHeader::addWords(const tstring& text) {
  TextRenderer::Paragraph::addWords(text, captionFont, GetSysColor(COLOR_BTNTEXT));
}

bool FrameHeader::updateLayout(HDC dc, int paragraphWidth, int x, int y, int height) {
  CRect r;
  GetClientRect(r);
  MapWindowPoints(GetParent(), r);
  m_textHeight = doLayout(dc, paragraphWidth);
  if (height != r.Height() || paragraphWidth != r.Width() || x != r.left || y != r.top) {
    MoveWindow(x, y, paragraphWidth, height);
    return true;
  }
  else {
    return false;
  }
}

void FrameHeader::updateLayout() {
  CRect r;
  GetClientRect(r);
  CClientDC dc(m_hWnd);
  m_textHeight = doLayout((HDC) dc, r.Width());
  Invalidate();
}

bool FrameHeader::updateLayout(int x, int y, int paragraphWidth) {
  CClientDC dc(m_hWnd);
  return updateLayout(dc, paragraphWidth, x, y, m_headerHeight);
}

void FrameHeader::setText(const TextRenderer::WordKey& wordKey, const tstring& text) {
  if (TextRenderer::Paragraph::setText(wordKey, text)) {
    updateLayout();
  }
}

LRESULT FrameHeader::onCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
  CDCHandle dc((HDC) wParam);
  dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
  dc.SetTextColor(GetSysColor(::IsWindowEnabled((HWND) lParam) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
  return (LRESULT) GetSysColorBrush(COLOR_BTNFACE);
}
