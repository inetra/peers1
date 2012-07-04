#pragma once
#include "webbrowser/WTLBrowserview.h"
#include "../windows/resource.h"
#include "PictureExWnd.h"

class AdviceBrowser : public CWTLBrowserView {
private:
	string m_currentURL;
protected:
	virtual DWORD getDocHostFlags();
	virtual BOOL OnBeforeNavigate2(IDispatch* /*pDisp*/, const String& szURL, DWORD /*dwFlags*/, const String& /*szTargetFrameName*/, CSimpleArray<BYTE>& /*pPostedData*/, const String& /*szHeaders*/);
	virtual void OnNavigateComplete2(IDispatch* /*pDisp*/, const String& /*szURL*/);
public:
	void navigate(const string& url);
};

class AdviceFrame:
  public MDITabChildWindowImpl<AdviceFrame, IDI_PEERS_ADVICE>, 
  public WindowSortOrder,
  public StaticFrame<AdviceFrame, ResourceManager::MENU_ADVICE_WINDOW>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("AdviceFrame"), IDI_PEERS_ADVICE, 0, COLOR_3DFACE);

	AdviceFrame();
	virtual ~AdviceFrame();

	virtual bool isLargeIcon() const { return true; }

	virtual WindowSortOrders getSortOrder()  {
		return SORT_ORDER_ADVICE;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);

	typedef MDITabChildWindowImpl<AdviceFrame, IDI_PEERS_ADVICE> baseClass;
	BEGIN_MSG_MAP(AdviceFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

private:
	AdviceBrowser m_browser;
	CPictureExWnd i_banner;
};
