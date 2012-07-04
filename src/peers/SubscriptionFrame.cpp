#include "stdafx.h"
#include "SubscriptionFrame.h"
#include "PeersUtils.h"
#include "../windows/HubFrame.h"

const string SubscriptionBrowser::m_subscriptionSite = "http://peersdata.cn.ru";
const string SubscriptionBrowser::m_passportUrl = m_subscriptionSite + "/passport/";

DWORD SubscriptionBrowser::getDocHostFlags() {
	return CWTLBrowserView::getDocHostFlags() & ~DOCHOSTUIFLAG_SCROLL_NO;
}

void SubscriptionBrowser::navigate(const string& url) {
	m_currentURL = url;
	Navigate2(Text::acpToWide(m_currentURL).c_str());
}

BOOL SubscriptionBrowser::OnBeforeNavigate2(IDispatch* /*pDisp*/, const String& szURL, DWORD /*dwFlags*/, const String& /*szTargetFrameName*/, CSimpleArray<BYTE>& /*pPostedData*/, const String& /*szHeaders*/) {
	const tstring urlT = tstring(szURL);
	const string url = Text::wideToAcp(urlT);
	dcdebug("SubscriptionBrowser::OnBeforeNavigate2(%s)\n", url.c_str());
	if (WinUtil::handleLink(urlT)) {
		return TRUE;
	}
	string allowedUrls[] = {
		m_passportUrl,
		m_subscriptionSite + "/peers/whylink"
	};
	for (int i = 0; i < COUNTOF(allowedUrls); ++i) {
		if (Util::startsWith(url, allowedUrls[i])) {
			return FALSE;
		}
	}
	ShellExecute(m_hWnd, _T("open"), szURL, NULL, NULL, SW_SHOWMAXIMIZED);
	return TRUE;
}

void SubscriptionBrowser::OnNavigateComplete2(IDispatch* /*pDisp*/, const String& /*szURL*/) {
	return;
}

SubscriptionFrame::SubscriptionFrame()
{
}

SubscriptionFrame::~SubscriptionFrame()
{
}

LRESULT SubscriptionFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	m_header.Create(m_hWnd);
	m_header.addWords(TSTRING(MENU_SUBSCRIPTION_WINDOW));
	if (m_browser.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)) {
		if (m_browser.isBrowserOK()) {
			string post = Util::emptyString;
			pair<string,string> credentials = findCredentials();
			if (!credentials.first.empty()) {
				post += "login=" + credentials.first;
				if (!credentials.second.empty()) {
					post += "&passwd=" + credentials.second;
				}
			}
			m_browser.Navigate2(Text::toT(SubscriptionBrowser::m_passportUrl).c_str(), 0, NULL, post.c_str(), post.length(), _T("Content-Type: application/x-www-form-urlencoded\r\n"));
		}
	}
	bHandled = FALSE;
	return TRUE;
}

void SubscriptionFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	m_header.updateLayout(rect.left, rect.top, rect.right);
	RECT headerRect;
	m_header.GetWindowRect(&headerRect);
	rect.top += headerRect.bottom - headerRect.top;
	if (m_browser.IsWindow()) {
		m_browser.MoveWindow(&rect);
	}
}

pair<string,string> SubscriptionFrame::findCredentials() {
	vector<MDIContainer::Window> windows = MDIContainer::list();
	for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
		HubFrame *frame = dynamic_cast<HubFrame*>(*i);
		if (frame) {
			Client* client = frame->getClient();
			string hubUrl = client->getHubUrl();
			if (Util::findSubString(hubUrl, PeersUtils::PEERS_HUB) != string::npos) {
				return make_pair(client->getCurrentNick(), client->getPassword());
			}
		}
	}
	return make_pair(Util::emptyString, Util::emptyString);
}
