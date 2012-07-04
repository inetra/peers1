#include "stdafx.h"
#include "../client/PME.h"
#include "FormattedText.h"
#include "ControlAdjuster.h"
#include "CaptionFont.h"

FormattedText::FormattedText() {
  setHorizontalGap(4);
}

LRESULT FormattedText::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  return 0;
}

LRESULT FormattedText::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_3DFACE));
  dc.SetBkColor(GetSysColor(COLOR_3DFACE));    
  draw((HDC) dc, 0, max(0, r.Height() - m_textHeight) / 2);
  return 0;
}

void FormattedText::addWords(const tstring& text, HFONT font) {
  COLORREF fontColor = GetSysColor(COLOR_WINDOWTEXT);
  HFONT currentFont = font;
  StringTokenizer<tstring> st(text, _T(' '));
  PME colorPattern("^\\[[0-9A-F]{6}\\]$");
  dcassert(colorPattern.IsValid());
  for (WStringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
    const tstring word = *i;
    if (!word.empty() && word.length() == 9 && word[0] == _T('[') && word[1] == _T('#') && word[8] == ']') {
      int red, green, blue;
      if (swscanf(word.c_str(), _T("[#%2x%2x%2x]"), &red, &green, &blue) == 3) {
        fontColor = RGB(red, green, blue);
      }
    }
    else if (word == _T("[BOLD]")) {
      if (!boldFont) {
        boldFont.Attach(CaptionFont(font, CaptionFont::BOLD).Detach());
      }
      currentFont = boldFont;
    }
    else if (word == _T("[NORMAL]")) {
      currentFont = font;
    }
    else {
      addWord(word, currentFont, fontColor);
    }
  }
}

bool FormattedText::updateLayout(HDC dc, int paragraphWidth, int x, int y) {
  CRect r;
  GetClientRect(r);
  MapWindowPoints(GetParent(), r);
  m_textHeight = doLayout(dc, paragraphWidth);
  if (m_textHeight != r.Height() || paragraphWidth != r.Width() || x != r.left || y != r.top) {
    MoveWindow(x, y, paragraphWidth, m_textHeight);
    return true;
  }
  else {
    return false;
  }
}

void FormattedText::updateLayout() {
  CRect r;
  GetClientRect(r);
  CClientDC dc(m_hWnd);
  m_textHeight = doLayout((HDC) dc, r.Width());
  Invalidate();
}

bool FormattedText::updateLayout(int x, int y, int paragraphWidth) {
  CClientDC dc(m_hWnd);
  return updateLayout(dc, paragraphWidth, x, y);
}
