#include "stdafx.h"
#include "CaptionFont.h"

CaptionFont::CaptionFont(FontOptions options, int sizeMul, int sizeDiv) {
  LOGFONT logFont;
  GetObject(WinUtil::font, sizeof(LOGFONT), &logFont);
  logFont.lfHeight = logFont.lfHeight * sizeMul / sizeDiv;
  logFont.lfWeight = options & BOLD ? FW_BOLD : FW_NORMAL;
  CreateFontIndirect(&logFont);
}

CaptionFont::CaptionFont(HFONT baseFont, FontOptions options) {
  LOGFONT logFont;
  GetObject(baseFont, sizeof(LOGFONT), &logFont);
  logFont.lfWeight = options & BOLD ? FW_BOLD : FW_NORMAL;
  CreateFontIndirect(&logFont);
}

CaptionFont::CaptionFont(HFONT baseFont, int sizeMul, int sizeDiv) {
  LOGFONT logFont;
  GetObject(baseFont, sizeof(LOGFONT), &logFont);
  CClientDC dc((HWND) 0);
  logFont.lfHeight = logFont.lfHeight * sizeMul / sizeDiv;
  CreateFontIndirect(&logFont);
}
