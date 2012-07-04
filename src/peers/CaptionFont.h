#pragma once

class CaptionFont : public CFont {
public:
  enum FontOptions { NORMAL = 0, BOLD = 1 };
  CaptionFont(FontOptions fontOptions = NORMAL, int sizeMul = 3, int sizeDiv = 2);
  CaptionFont(HFONT baseFont, FontOptions fontOptions = NORMAL);
  CaptionFont(HFONT baseFont, int sizeMul, int sizeDiv);
};
