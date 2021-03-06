// WTLBrowserView.h : interface of the CWTLBrowserView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WTLBROWSERVIEW_H__27BEDF70_F34D_4698_A226_534A3D32E98B__INCLUDED_)
#define AFX_WTLBROWSERVIEW_H__27BEDF70_F34D_4698_A226_534A3D32E98B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Browser.h"

class CWTLBrowserView : public CWindowImpl<CWTLBrowserView, CAxWindow>, public CWebBrowser2<CWTLBrowserView>
{
private:
  CComObjectStackEx<CAxHostWindow> m_hostWindow;
  tstring m_savedNavigatingSound;
protected:
	virtual DWORD getDocHostFlags();
public:
  DECLARE_WND_SUPERCLASS(NULL, CAxWindow::GetWndClassName())

  BOOL PreTranslateMessage(MSG* pMsg);

  BEGIN_MSG_MAP(CWTLBrowserView)
    MESSAGE_HANDLER(WM_CREATE, onCreate)
    CHAIN_MSG_MAP(CWebBrowser2<CWTLBrowserView>)
  END_MSG_MAP()

  LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  void ReleaseAll() {
    m_hostWindow.ReleaseAll();
  }

  virtual BOOL OnBeforeNavigate2(IDispatch* /*pDisp*/, const String& /*szURL*/, DWORD /*dwFlags*/, const String& /*szTargetFrameName*/, CSimpleArray<BYTE>& /*pPostedData*/, const String& /*szHeaders*/);
  virtual void OnNavigateComplete2(IDispatch* /*pDisp*/, const String& /*szURL*/);

};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WTLBROWSERVIEW_H__27BEDF70_F34D_4698_A226_534A3D32E98B__INCLUDED_)
