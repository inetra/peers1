#pragma once
#include "EditWithIcon.h"

class CClearButton: public CWindowImpl<CClearButton, CButton> {
public:
  BEGIN_MSG_MAP(CClearButton)
    MESSAGE_HANDLER(OCM_DRAWITEM, onDrawItem);
  END_MSG_MAP()

  LRESULT onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

class CFilterEdit : public CEditWithIcon {
private:
  bool clearButtonVisible;
  CClearButton clearButton;
  /* на входе клиентская область окна, на выходе - где разместить кнопку очистки */
  void updateClearButtonRect(CRect &r);
protected:
  virtual void onSize();
  virtual void updateEditRect(CRect& r);
public:
  CFilterEdit():CEditWithIcon(IDB_USER_LIST_FILTER), clearButtonVisible(false) { }

  BEGIN_MSG_MAP(CFilterEdit)
    COMMAND_HANDLER(IDC_USER_LIST_FILTER_CLEAR, BN_CLICKED, onClearButtonClick)
    REFLECT_NOTIFICATIONS()
    CHAIN_MSG_MAP(CEditWithIcon)
  END_MSG_MAP()

  LRESULT onClearButtonClick(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);

  void showClearButton();
  void hideClearButton();
  void setClearButtonVisible(bool visible) {
    if (visible) {
      showClearButton();
    }
    else {
      hideClearButton();
    }
  }
};
