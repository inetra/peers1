#pragma once
#include "TextRenderer.h"
#include "CaptionFont.h"

#define CHAIN_PARENT_COMMANDS() \
	if(uMsg == WM_COMMAND) \
		::SendMessage(GetParent(), uMsg, wParam, lParam);

class FrameHeader : public CWindowImpl<FrameHeader>, private TextRenderer::Paragraph {
private:
  CaptionFont captionFont;
  int m_leftMargin;
  int m_textHeight;
  int m_headerHeight;
  bool updateLayout(HDC dc, int paragraphWidth, int x, int y, int height);
public:
  FrameHeader();

  BEGIN_MSG_MAP(FrameHeader)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColorStatic);
    REFLECT_NOTIFICATIONS();
    CHAIN_PARENT_COMMANDS();
  END_MSG_MAP();
    
  LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }

  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  LRESULT onCtlColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);  

  int getLeftMargin() const {
    return m_leftMargin;
  }

  int getPreferredHeight() const {
    return m_headerHeight;
  }

  int getTextWidth() const {
    return TextRenderer::Paragraph::getWidth();
  }

  /* добавляет текст */
  void addWords(const tstring& text);

  /* меняет положение и размеры окна */
  bool updateLayout(int x, int y, int paragraphWidth);

  /* пересчитывает размеры текста */
  void updateLayout();

  /* меняет конкретное слово в тексте и пересчитывает размеры текста */
  void setText(const TextRenderer::WordKey& wordKey, const tstring& text);

};
