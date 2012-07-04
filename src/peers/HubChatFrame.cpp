#include "stdafx.h"
#include "PeersUtils.h"
#include "HubChatFrame.h"

HubChatFrame::HubChatFrame(HubFrame *hubFrame, ChatControlListener* chatListener, UserListControlListener* userListListener):
m_hubFrame(hubFrame),
m_chatListener(chatListener),
m_chatControl(this),
m_userListControl(userListListener)
{
}

LRESULT HubChatFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  m_chatControl.Create(m_hWnd);
  m_userListControl.Create(m_hWnd);
  SetSplitterPanes(m_chatControl.m_hWnd, m_userListControl.m_hWnd, false);
  m_nProportionalPos = 5000;
  SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
  m_userListControl.initColumns(Text::toT(m_chatListener->getClient()->getHubUrl()));
  SettingsManager::getInstance()->addListener(this);
  m_userListControl.reloadUserList();
  PostMessage(WPM_AFTER_CREATE);
  bHandled = FALSE; /* pass to splitter */
  return 0;
}

LRESULT HubChatFrame::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_BTNFACE));
  return 0;
}

LRESULT HubChatFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  SettingsManager::getInstance()->removeListener(this);
  bHandled = FALSE;
  return 0;
}

void HubChatFrame::onActivate() {
  m_chatControl.SetFocus();
}

LRESULT HubChatFrame::onAfterCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_chatControl.SetFocus();
  return 0;
}

void HubChatFrame::UpdateLayout(BOOL /*bResizeBars*/) {
  RECT r;
  GetClientRect(&r);
  SetSplitterRect(&r);
}

void HubChatFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
  m_userListControl.updateColors();
  m_chatControl.updateColors();
  RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

WindowSortOrders HubChatFrame::getSortOrder() {
	return Util::findSubString(getClient()->getHubUrl(), PeersUtils::PEERS_HUB) != string::npos ? SORT_ORDER_PEERS_CHAT : SORT_ORDER_HUB_OTHER;
}
