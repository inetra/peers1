#pragma once
#include "FrameHeader.h"
#include "webbrowser/WTLBrowserview.h"
#include "../windows/resource.h"

class SubscriptionBrowser : public CWTLBrowserView {
private:
	const static string m_subscriptionSite;
	const static string m_passportUrl;
	string m_currentURL;
	friend class SubscriptionFrame;
protected:
	virtual DWORD getDocHostFlags();
	virtual BOOL OnBeforeNavigate2(IDispatch* /*pDisp*/, const String& szURL, DWORD /*dwFlags*/, const String& /*szTargetFrameName*/, CSimpleArray<BYTE>& /*pPostedData*/, const String& /*szHeaders*/);
	virtual void OnNavigateComplete2(IDispatch* /*pDisp*/, const String& /*szURL*/);
public:
	void navigate(const string& url);
};

class SubscriptionFrame:
  public MDITabChildWindowImpl<SubscriptionFrame, IDI_PEERS_SUBSCRIPTION>, 
  public WindowSortOrder,
  public StaticFrame<SubscriptionFrame, ResourceManager::MENU_SUBSCRIPTION_WINDOW>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("SubscriptionFrame"), IDI_PEERS_SUBSCRIPTION, 0, COLOR_3DFACE);

	SubscriptionFrame();
	virtual ~SubscriptionFrame();

	virtual bool isLargeIcon() const { return true; }

	virtual WindowSortOrders getSortOrder()  {
		return SORT_ORDER_SUBSCRIPTIONS;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);

	typedef MDITabChildWindowImpl<SubscriptionFrame, IDI_PEERS_SUBSCRIPTION> baseClass;
	BEGIN_MSG_MAP(SubscriptionFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

private:
	FrameHeader m_header;
	SubscriptionBrowser m_browser;
	pair<string,string> findCredentials();
};
