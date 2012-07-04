#pragma once
#include <atlctrlx.h>

class CHelpHyperLink : public CHyperLinkImpl<CHelpHyperLink> {
private:
  CIcon helpIcon;
  CFont smallFont;
public:
  CHelpHyperLink(): CHyperLinkImpl(HLINK_UNDERLINEHOVER | HLINK_NOTOOLTIP) { }
  void Init();
  void DoEraseBackground(CDCHandle dc);
  void DoPaint(CDCHandle dc);
  bool CalcLabelRect();
  bool GetIdealSize(int& cx, int& cy, int clientWidth) const;
};
