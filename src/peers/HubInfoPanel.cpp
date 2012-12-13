#include "stdafx.h"
#include "../client/ShareManager.h"
#include "HubInfoPanel.h"
#include "ControlAdjuster.h"
#include "../windows/resource.h"

#define NICK_BLOCK		1
#define STATUS_BLOCK	2
#define SHARE_BLOCK		4

#define ALL_BLOCKS   0xFF

#define VGAP 16
#define HGAP 16
#define HPADDING 16
#define VPADDING 16
#define BUTTON_HEIGHT 36
#define TEXT_COLOR RGB(0,0,0)

#define BANNER_WIDTH  600
#define BANNER_HEIGHT 90

#define BANNER_CREATE_TIMER_ID			1
#define BANNER_CREATE_TIMER_INTERVAL	1000

LRESULT HubInfoBlock::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_3DFACE));
  dc.SetBkColor(GetSysColor(COLOR_3DFACE));
  draw((HDC) dc, 0, 0);
  return 0;
}

int HubInfoBlock::getHeight() const {
  CRect r;
  GetWindowRect(r);
  return r.Height();
}

bool HubInfoBlock::updateLayout(CDCHandle dc, int paragraphWidth, int x, int& y, bool invalidateAnyway) {
  CRect r;
  GetClientRect(r);
  MapWindowPoints(GetParent(), r);
  const int h = doLayout(dc, paragraphWidth);
  if (h != r.Height() || paragraphWidth != r.Width() || x != r.left || y != r.top) {
    MoveWindow(x, y, paragraphWidth, h);
    y += h;
    return true;
  }
  else {
    if (invalidateAnyway) {
      Invalidate();
    }
    y += h;
    return false;
  }
}

HubInfoPanel::HubInfoPanel(HubInfoPanelController *controller): 
m_controller(controller),
m_bannerCreateTimerStarted(false),
captionFont(CaptionFont::BOLD),
greenFont(CaptionFont::BOLD, 5, 4),
textFont(CaptionFont::NORMAL, 1, 1),
boldFont(CaptionFont::BOLD, 5, 4),
buttonFont(CaptionFont::NORMAL, 5, 4)
{
	showBanner = false;
}

HWND HubInfoPanel::Create(HWND hWndParent) {
  return CWindowImpl<HubInfoPanel>::Create(hWndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

LRESULT HubInfoPanel::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_btnReconnect.Create(IDI_HUB_RECONNECT, m_hWnd, _T("Переподключиться"), ID_FILE_RECONNECT);
  m_btnReconnect.setHintText(TSTRING(HUB_INFO_RECONNECT_HINT));
  m_btnConfigure.Create(IDI_HUB_CONFIGURE, m_hWnd, _T("Настройки"), ID_FILE_SETTINGS);
  m_btnConfigure.setHintText(TSTRING(HUB_INFO_CONFIGURE_HINT));
  m_btnAddFiles.Create(IDI_HUB_FILE_ADD, m_hWnd, _T("Добавить папки"), IDC_FILELIST_ADD_FILE);
  m_btnAddFiles.setHintText(TSTRING(HUB_INFO_ADD_FILES_HINT));
  m_btnOpenFileList.Create(IDI_HUB_FILELIST, m_hWnd, _T("Мой список"), IDC_OPEN_MY_LIST);
  m_btnOpenFileList.setHintText(TSTRING(HUB_INFO_OPEN_FILE_LIST_HINT));
  m_btnRefreshFileList.Create(IDI_HUB_REFRESH_FILELIST, m_hWnd, _T("Обновить список"), IDC_REFRESH_FILE_LIST);
  m_btnRefreshFileList.setHintText(TSTRING(HUB_INFO_REFRESH_FILE_LIST_HINT));
  FlatIconButton* const buttons[] = { &m_btnReconnect, &m_btnConfigure, &m_btnAddFiles, &m_btnOpenFileList, &m_btnRefreshFileList };
  for (int i = 0; i < COUNTOF(buttons); ++i) {
    FlatIconButton* const button = buttons[i];
    button->setHighLightColor(HOVER_COLOR);
    button->setHighLightBorder(true);
    button->SetFont(buttonFont);
  }
  pMyNick.Create(m_hWnd);
  pStatus.Create(m_hWnd);
  pMyShare.Create(m_hWnd);
  // my nick
  pMyNick.addWord(_T("Вы"), captionFont, TEXT_COLOR);
  myNick = pMyNick.addWord(Util::emptyStringT, captionFont, TEXT_COLOR);
  // Статус подключения
  pStatus.addWord(_T("Ваш статус"), boldFont, TEXT_COLOR);
  statusName = pStatus.addWord(Util::emptyStringT, greenFont, GREEN_COLOR);
  statusMode = pStatus.addWord(Util::emptyStringT, textFont, TEXT_COLOR);
  // my share
  pMyShare.addWord(_T("Объем ваших файлов"), boldFont, TEXT_COLOR);
  myShareSize = pMyShare.addWord(Util::emptyStringT, greenFont, GREEN_COLOR);
  pMyShare.addWord(_T("—"), textFont, TEXT_COLOR);
  mySharePercent = pMyShare.addWord(Util::emptyStringT, textFont, TEXT_COLOR);
  pMyShare.addWord(_T("от среднего"), textFont, TEXT_COLOR);
  i_banner.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_OWNERDRAW | SS_ICON | SS_NOTIFY);
  if (!BOOLSETTING(MINIMIZE_ON_STARTUP)) {
    startTimer();
  }
  return 0;
}

LRESULT HubInfoPanel::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (!WinUtil::isAppMinimized() && WinUtil::MDIGetActive() == GetParent()) {
		createBannerNow();
	}
	else {
		destroyBannerNow();
	}
	KillTimer(BANNER_CREATE_TIMER_ID);
	m_bannerCreateTimerStarted = false;
	return 0;
}

void HubInfoPanel::startTimer() {
	if (m_bannerCreateTimerStarted) {
		KillTimer(BANNER_CREATE_TIMER_ID);
	}
	else {
		m_bannerCreateTimerStarted = true;
	}
	SetTimer(BANNER_CREATE_TIMER_ID, BANNER_CREATE_TIMER_INTERVAL, NULL);
}

void HubInfoPanel::createBannerNow() {
	if (!showBanner) {
		showBanner = true;
		USES_CONVERSION;
		i_banner.LoadAdRiverXMLBanner(BANNER2_SID, BANNER2_SZ, BANNER2_BN, BANNER2_BT, BANNER2_PZ);
	}
}

void HubInfoPanel::destroyBannerNow() {
	if (showBanner) {
		//i_banner.DestroyWindow();
		//i_banner.Detach();
		i_banner.UnLoadAdRiverXMLBanner();
		showBanner = false;
	}
}

int HubInfoPanel::getBannerWidth() const {
  return BANNER_WIDTH + 2 * GetSystemMetrics(SM_CXEDGE);
}

int HubInfoPanel::getMinHeight() const {
	CRect r;
	m_btnAddFiles.GetWindowRect(r);
	::MapWindowPoints(NULL, *this, (LPPOINT)&r, 2);
	return r.bottom + BANNER_HEIGHT;
}

LRESULT HubInfoPanel::onForwardCommandToParent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
  return ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(wID, wNotifyCode), (LPARAM) hWndCtl);
}

LRESULT HubInfoPanel::onForwardCommandToMain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
  return ::SendMessage(WinUtil::mainWnd, WM_COMMAND, MAKEWPARAM(wID, wNotifyCode), (LPARAM) hWndCtl);
}

void HubInfoPanel::updateData() {
  // TODO можно передавать параметры какие именно поля обновлять.
  const Client* client = m_controller->getClient();
  int nickChanges = 0;
  if (pMyNick.setText(myNick, Text::toT(client->getMyNick()))) ++nickChanges;
  int statusChanges = 0;
  if (pStatus.setText(statusName, client->getState() == Client::STATE_NORMAL ? _T("Подключен") : _T("Не подключен"))) ++statusChanges;
  if (pStatus.setText(statusMode, client->isActive() ? _T("(активный режим)") : _T("(пассивный режим)"))) ++statusChanges;
  int shareChanges = 0;
  int64_t myShareSize = ShareManager::getInstance()->getSharedSize();
  int64_t totalShareSize = client->getAvailable();
  if (pMyShare.setText(this->myShareSize, Util::formatBytesW(myShareSize))) ++shareChanges;
  if (totalShareSize > myShareSize && client->getUserCount() > 0) {
	  TCHAR buffer[16];
	  int64_t averageShareSize = totalShareSize / client->getUserCount();
	  if (averageShareSize > 0) {
		  int percentInt = (int) (myShareSize * 100 / averageShareSize);
		  int percentFraction = (int) ((myShareSize * 1000) / averageShareSize - (percentInt * 10));
		  _stprintf(buffer, _T("%d.%01d%%"), percentInt, percentFraction);
		  if (pMyShare.setText(mySharePercent, buffer)) ++shareChanges;
	  }
	  else {
		  if (pMyShare.setText(mySharePercent, _T("?"))) ++shareChanges;
	  }
  }
  else {
	  if (pMyShare.setText(mySharePercent, _T("?"))) ++shareChanges;
  }
  if (nickChanges || statusChanges || shareChanges) {
    updateLayout(
      (nickChanges ? NICK_BLOCK : 0) |
      (statusChanges ? STATUS_BLOCK : 0) | 
      (shareChanges ? SHARE_BLOCK : 0),
      true
    );
  }
}

int HubInfoPanel::updateLayout(int blocks, bool invalidateAnyway) {
  CRect r;
  GetClientRect(r);
  r.InflateRect(-HPADDING, -VPADDING);
  return updateLayout(r, blocks, invalidateAnyway);
}

int HubInfoPanel::updateLayout(const CRect& r, int blocks, bool invalidateAnyway) {
  CClientDC dc(m_hWnd);
  const HFONT oldFont = dc.SelectFont(captionFont);
  const int width2 = (r.Width() - 2 * HGAP) / 3;
  int y = r.top;
  if (blocks & NICK_BLOCK) {
	  if (pMyNick.updateLayout((HDC) dc, width2, r.left, y, invalidateAnyway)) {
		  blocks |= ~NICK_BLOCK;
	  }
  }
  y = r.top;
  pStatus.updateLayout((HDC) dc, r.Width() - width2, r.left + width2 + HGAP, y, invalidateAnyway);
  pMyShare.updateLayout((HDC) dc, r.Width() - width2, r.left + width2 + HGAP, y, invalidateAnyway);
  
  FlatIconButton* buttons[] = {
	  &m_btnReconnect,
	  &m_btnConfigure,
	  &m_btnAddFiles,
	  &m_btnOpenFileList,
	  &m_btnRefreshFileList
  };
  LONG buttonWidth = buttons[0]->getPreferredSize().cx;
  for (int i = 1; i < COUNTOF(buttons); ++i) {
	  buttonWidth = max(buttonWidth, buttons[i]->getPreferredSize().cx);
  }

  y = r.top + max(pMyNick.getHeight(), pStatus.getHeight() + pMyShare.getHeight()) + VGAP;
  m_btnReconnect.MoveWindow(r.left, y, buttonWidth, BUTTON_HEIGHT);
  m_btnConfigure.MoveWindow(r.left, y + BUTTON_HEIGHT + VGAP, buttonWidth, BUTTON_HEIGHT);

  m_btnOpenFileList.MoveWindow(r.left + width2 + HGAP, y, buttonWidth, BUTTON_HEIGHT);
  m_btnAddFiles.MoveWindow(r.left + width2 + HGAP, y + BUTTON_HEIGHT + VGAP, buttonWidth, BUTTON_HEIGHT);

  m_btnRefreshFileList.MoveWindow(r.left + 2 * (width2 + HGAP), y, buttonWidth, BUTTON_HEIGHT);
  dc.SelectFont(oldFont);
  return y + BUTTON_HEIGHT * 3;
}

LRESULT HubInfoPanel::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CRect r;
  GetClientRect(r);
  r.InflateRect(-HPADDING, -VPADDING);
  updateLayout(r, ALL_BLOCKS, false);
//  if (showBanner) {
    r.InflateRect(HPADDING, VPADDING);
    r.left += 2 * GetSystemMetrics(SM_CXEDGE);
    i_banner.MoveWindow(
      max(r.left, (r.left + r.right - BANNER_WIDTH) / 2),
      r.Height() - BANNER_HEIGHT,
      BANNER_WIDTH,
      BANNER_HEIGHT);
  //}
  return 0;
}

LRESULT HubInfoPanel::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_3DFACE));
  return 0;
}

HubInfoNumberPanel::HubInfoNumberPanel(HubInfoPanelController *controller):
m_controller(controller),
textFont(CaptionFont::NORMAL, 1, 1),
greenFont(CaptionFont::BOLD, 5, 4)
{
}

HWND HubInfoNumberPanel::Create(HWND hWndParent) {
  return CWindowImpl<HubInfoNumberPanel>::Create(hWndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

LRESULT HubInfoNumberPanel::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  addWords(_T("Сейчас в сети:"), textFont, TEXT_COLOR);
  totalUserCount = addWord(Util::emptyStringT, greenFont, GREEN_COLOR);
  addWord(_T("человек расшарили"), textFont, TEXT_COLOR);
  totalSharedSize = addWord(Util::emptyStringT, greenFont, GREEN_COLOR);
  return 0;
}

void HubInfoNumberPanel::updateData() {
  int changes = 0;
  const Client* client = m_controller->getClient();
  if (setText(totalUserCount, Text::toT(Util::toString(client->getUserCount())))) ++changes;
  if (setText(totalSharedSize, Util::formatBytesW(client->getAvailable()))) ++changes;
  if (changes) {
	  CRect r;
	  GetClientRect(r);
	  doLayout(r.Width());
	  Invalidate();
  }
}

LRESULT HubInfoNumberPanel::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CPaintDC dc(m_hWnd);
	CRect r;
	GetClientRect(r);
	dc.FillRect(r, GetSysColorBrush(COLOR_3DFACE));
	dc.SetBkColor(GetSysColor(COLOR_3DFACE));
	draw((HDC)dc, H_PADDING, V_PADDING);
	return 0;
}
