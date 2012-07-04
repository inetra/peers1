#include "stdafx.h"
#include "../windows/resource.h"
#include "EgoPhoneLauncher.h"
#include "webbrowser/WTLBrowserview.h"

class EgoPhoneInstallDialog : public CDialogImpl<EgoPhoneInstallDialog> {
private:
  RECT getBrowserRect() const {
    RECT r;
    GetDlgItem(IDC_DOWNLOAD_PROGRESS).GetWindowRect(&r);
    ScreenToClient(&r);
    r.bottom = r.top - r.left;
    r.top = r.left;
    return r;
  }
   
  CWTLBrowserView m_info;
public:
  enum { IDD = IDD_EGOPHONE_INSTALL };

  BEGIN_MSG_MAP(EgoPhoneInstallDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnInstallCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    CenterWindow();
    RECT infoRect = getBrowserRect();
    if (m_info.Create(m_hWnd, infoRect, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)) {
      if (m_info.isBrowserOK()) {
        m_info.LoadFromResource(IDR_EGOPHONE_HTML);
      }
    }
    return TRUE;
  }

  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

  LRESULT OnInstallCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    GetDlgItem(IDC_DOWNLOAD_PROGRESS).ShowWindow(SW_SHOW);
    GetDlgItem(IDOK).EnableWindow(FALSE);
    return 0;
  }  

};

// returns true if started
bool EgoPhoneLauncher::startIfInstalled() {
  bool result = false;
  CRegKey key;
  if (key.Open(HKEY_CURRENT_USER, _T("Software\\¡˝Í‡Ô »“\\›„Ó‘ÓÌ")) == ERROR_SUCCESS) {
    TCHAR szApplicationPath[1024];
    DWORD dwSize = sizeof(szApplicationPath);
    if (key.QueryValue(szApplicationPath, _T("ApplicationPath"), &dwSize) == ERROR_SUCCESS && File::exists(Text::fromT(szApplicationPath))) {
      STARTUPINFO si;
      memset(&si, 0, sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_FORCEOFFFEEDBACK;
      PROCESS_INFORMATION pi;
      memset(&pi, 0, sizeof(PROCESS_INFORMATION));      
      BOOL execResult = CreateProcess(NULL, szApplicationPath, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
      if (execResult) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        result = true;
      }
    }
    key.Close();
  }
  return result;
}

void EgoPhoneLauncher::execute() {
  if (startIfInstalled()) {
    return;
  }
  EgoPhoneInstallDialog dialog;
  dialog.DoModal(m_parentWnd);
}
