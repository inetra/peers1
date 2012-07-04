#pragma once
#include "EditIcon.h"

class CEditWithIcon : public CWindowImpl<CEditWithIcon, CEdit> {
public:
  DECLARE_WND_SUPERCLASS(NULL, _T("edit"))
  
  BEGIN_MSG_MAP(CEditWithIcon)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_SIZE, onSize);
  END_MSG_MAP();

  LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
public:
  CEditWithIcon(ATL::_U_STRINGorID resourceId): m_resourceId(resourceId) { }
  tstring getText() const;
protected:
  void setEditRect();
  virtual void onSize();
  virtual void updateEditRect(CRect& r);
private:
  ATL::_U_STRINGorID m_resourceId;
  CEditIcon icon;
};
