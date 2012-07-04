#pragma once

class UpdateRestartConfirmDlg : public CSimpleDialog<IDD_UPDATE_RESTART_CONFIRM,true> {
private:
  typedef CSimpleDialog<IDD_UPDATE_RESTART_CONFIRM,true> super;
public:
  BEGIN_MSG_MAP(UpdateRestartConfirmDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    CHAIN_MSG_MAP(super)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    SetWindowText(_T(APPNAME));
    return super::OnInitDialog(uMsg, wParam, lParam, bHandled);
  }

};
