// WTLBrowserView.cpp : implementation of the CWTLBrowserView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WTLBrowserView.h"

BOOL CWTLBrowserView::PreTranslateMessage(MSG* pMsg)
{
  if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
    (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
    return FALSE;

  // give HTML page a chance to translate this message
  return (BOOL)SendMessage(WM_FORWARDMSG, 0, (LPARAM)pMsg);
}

BOOL CWTLBrowserView::OnBeforeNavigate2(IDispatch* /*pDisp*/, const String& /*szURL*/, DWORD /*dwFlags*/, const String& /*szTargetFrameName*/, CSimpleArray<BYTE>& /*pPostedData*/, const String& /*szHeaders*/) {
  CRegKey key;
  if (key.Open(HKEY_CURRENT_USER, _T("AppEvents\\Schemes\\Apps\\Explorer\\Navigating\\.Current")) == ERROR_SUCCESS) {
    TCHAR szSoundKeyValue[255];
    DWORD dwSize = sizeof(szSoundKeyValue);
    if (key.QueryValue(szSoundKeyValue, NULL, &dwSize) == ERROR_SUCCESS) {
      m_savedNavigatingSound.assign(szSoundKeyValue, dwSize);
      key.SetValue(_T("")); //Setting default value to the key
    }
    key.Close();
  }
  return FALSE;
}

void CWTLBrowserView::OnNavigateComplete2(IDispatch* /*pDisp*/, const String& /*szURL*/) {
  if (!m_savedNavigatingSound.empty()) {
    CRegKey key;
    if (key.Open(HKEY_CURRENT_USER, _T("AppEvents\\Schemes\\Apps\\Explorer\\Navigating\\.Current")) == ERROR_SUCCESS) {
      key.SetValue(m_savedNavigatingSound.c_str()); 
      m_savedNavigatingSound.clear();
    }
    key.Close();
  }
}

LRESULT CWTLBrowserView::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_hostWindow.SubclassWindow(m_hWnd);
  m_hostWindow.put_DocHostFlags(getDocHostFlags());
  m_hostWindow.put_AllowContextMenu(VARIANT_FALSE);
  HRESULT hr = createControl();
#ifdef _DEBUG
  ATLASSERT(SUCCEEDED(hr));
#else
  (void) hr;
#endif
  Silent = VARIANT_TRUE;
  RegisterAsDropTarget = VARIANT_FALSE;
  RegisterAsBrowser = VARIANT_FALSE;
  return 0;
}

DWORD CWTLBrowserView::getDocHostFlags() {
	return DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_OPENNEWWIN | DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK | DOCHOSTUIFLAG_DISABLE_UNTRUSTEDPROTOCOL;
}
