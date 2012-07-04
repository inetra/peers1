#pragma once
#include "FlatButton.h"

/*
чтобы работало рисование, нужно в родительском окне добавлять
REFLECT_NOTIFICATIONS();
*/

class FlatIconButton: public FlatButton {
private:
  CIcon m_icon;
  void drawDisabledIcon(CDCHandle dc, int x, int y);
protected:
  virtual void drawButton(CDCHandle dc, const CRect &r, bool selected);
  virtual CIcon& getCurrentIcon() { return m_icon; }
public:
  HWND Create(ATL::_U_STRINGorID resourceId, HWND hWndParent, LPCTSTR szWindowName, _U_MENUorID MenuOrID);
  virtual SIZE getPreferredSize();
};
