#pragma once
#include "TextRenderer.h"

class FormattedText : public CWindowImpl<FormattedText>, private TextRenderer::Paragraph {
private:
  int m_textHeight;
  CFont boldFont;
  bool updateLayout(HDC dc, int paragraphWidth, int x, int y);
public:
  FormattedText();

  BEGIN_MSG_MAP(FormattedText)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
  END_MSG_MAP();
    
  LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }

  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  int getTextWidth() const {
    return TextRenderer::Paragraph::getWidth();
  }

  int getTextHeight() const {
	  return m_textHeight;
  }

  /* добавляет текст */
  void addWords(const tstring& text, HFONT font);

  /* меняет положение и размеры окна */
  bool updateLayout(int x, int y, int paragraphWidth);

  void updateLayout();

};
