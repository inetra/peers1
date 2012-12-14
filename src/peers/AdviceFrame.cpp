#include "stdafx.h"
#include "AdviceFrame.h"

#define BANNER_WIDTH  600
#define BANNER_HEIGHT 90

DWORD AdviceBrowser::getDocHostFlags() {
	return CWTLBrowserView::getDocHostFlags() & ~DOCHOSTUIFLAG_SCROLL_NO;
}

void AdviceBrowser::navigate(const string& url) {
	m_currentURL = url;
	Navigate2(Text::acpToWide(m_currentURL).c_str());
}

BOOL AdviceBrowser::OnBeforeNavigate2(IDispatch* /*pDisp*/, const String& szURL, DWORD /*dwFlags*/, const String& /*szTargetFrameName*/, CSimpleArray<BYTE>& /*pPostedData*/, const String& /*szHeaders*/) {
#ifdef _DEBUG
	dcdebug("AdviceBrowser::OnBeforeNavigate2(%s)\n", Text::fromT(tstring(szURL)).c_str());
#endif
	tstring urlT = tstring(szURL);
	if (WinUtil::handleLink(urlT)) {
		return TRUE;
	}
	const string url = Text::wideToAcp(urlT);
	if (url == m_currentURL || url.substr(0, 4) != "http") {
		return FALSE;
	}
	ShellExecute(m_hWnd, _T("open"), szURL, NULL, NULL, SW_SHOWMAXIMIZED);
	return TRUE;
}

void AdviceBrowser::OnNavigateComplete2(IDispatch* /*pDisp*/, const String& /*szURL*/) {
	return;
}

AdviceFrame::AdviceFrame()
{
}

AdviceFrame::~AdviceFrame()
{
}

LRESULT AdviceFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (i_banner.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_OWNERDRAW | SS_ICON | SS_NOTIFY)) {
		USES_CONVERSION;
		i_banner.LoadAdRiverXMLBanner(BANNER1_SID, BANNER1_SZ, BANNER1_BN, BANNER1_BT, BANNER1_PZ);
	}

	if (m_browser.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)) {
		if (m_browser.isBrowserOK()) {
			m_browser.navigate(ADVICE_CONTENT);
		}
	}
	bHandled = FALSE;
	return TRUE;
}

void AdviceFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	if (i_banner.BannerLoaded()) {
		i_banner.MoveWindow(
			rect.left + (rect.right - rect.left)/2 - BANNER_WIDTH/2, 
			rect.top, 
			BANNER_WIDTH, 
			BANNER_HEIGHT);
		rect.top += BANNER_HEIGHT;
	} else {
		i_banner.MoveWindow(0, 0, 0, 0);
	}
	if (m_browser.IsWindow()) {
		m_browser.MoveWindow(&rect);
	}
}
