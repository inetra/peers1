#include "stdafx.h"
#include "PeersToolbar.h"
#include "../windows/QueueFrame.h"
#include "../windows/WaitingUsersFrame.h"
#include "../windows/FinishedFrame.h"
#include "../windows/FinishedULFrame.h"
#include "SearchFrmFactory.h"
#include "ControlAdjuster.h"
#include "AdviceFrame.h"
#include "SubscriptionFrame.h"
#include "../windows/resource.h"

#ifdef _DEBUG
#include "PostMessageDelayer.h"
#endif

#define TEXT_COLOR   RGB(255,255,255)
#define ORANGE_COLOR RGB(231,93,0)
#define HILITE_COLOR	HEX_RGB(0xF8931D)

#define LOGO_WIDTH				316
#define BUTTON_LEFT_MARGIN      LOGO_WIDTH
#define BUTTON_TOP_MARGIN       3
#define BUTTON_BOTTOM_MARGIN    2
#define BUTTON_H_GAP            16

PeersToolbar::PeersToolbar()
{
}

LRESULT PeersToolbar::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  CBitmap logo;
  if (logo.LoadBitmap(IDB_PEERS_MAIN_LOGO)) {
	  CDC bitmapDC;
	  bitmapDC.CreateCompatibleDC(dc);
	  const HBITMAP oldBitmap = bitmapDC.SelectBitmap(logo);
	  dc.BitBlt(0, 0, LOGO_WIDTH, getHeight(), bitmapDC, 0, 0, SRCCOPY);
	  dc.StretchBlt(LOGO_WIDTH, 0, r.right - LOGO_WIDTH, getHeight(), bitmapDC, LOGO_WIDTH - 60, 0, 60, getHeight(), SRCCOPY);
	  bitmapDC.SelectBitmap(oldBitmap);
  }
  else {
	  dc.FillRect(r, (HBRUSH) GetStockObject(WHITE_BRUSH));
  }
  return 0;
}

LRESULT PeersToolbar::onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  return 1;
}

HWND PeersToolbar::Create(HWND hWndParent) {
  return CWindowImpl<PeersToolbar>::Create(hWndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

LRESULT PeersToolbar::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_btnAdvice.Create(IDB_PEERS_TOOLBAR_BUTTON_NEWS, IDB_PEERS_TOOLBAR_BUTTON_NEWS_HOVER, m_hWnd, IDC_ADVICE_WINDOW);
  m_btnAdvice.setHintText(TSTRING(MENU_ADVICE_WINDOW));

  m_btnSubscriptions.Create(IDB_PEERS_TOOLBAR_BUTTON_SUBSCRIPTIONS, IDB_PEERS_TOOLBAR_BUTTON_SUBSCRIPTIONS_HOVER, m_hWnd, IDC_SUBSCRIPTIONS);
  m_btnSubscriptions.setHintText(TSTRING(MENU_SUBSCRIPTIONS));

  m_btnDownloadQueue.Create(IDB_PEERS_TOOLBAR_BUTTON_DOWNLOADS, IDB_PEERS_TOOLBAR_BUTTON_DOWNLOADS_HOVER, m_hWnd, IDC_QUEUE);
  m_btnDownloadQueue.setHintText(TSTRING(HINT_DOWNLOAD_QUEUE));

  m_btnFinishedDownloads.Create(IDB_PEERS_TOOLBAR_BUTTON_FINISHED_DOWNLOADS, IDB_PEERS_TOOLBAR_BUTTON_FINISHED_DOWNLOADS_HOVER, m_hWnd, IDC_FINISHED);
  m_btnFinishedDownloads.setHintText(TSTRING(HINT_FINISHED_DOWNLOADS));

  m_btnChat.Create(IDB_PEERS_TOOLBAR_BUTTON_CHAT, IDB_PEERS_TOOLBAR_BUTTON_CHAT_HOVER, m_hWnd, IDC_PEERS_TOOLBAR_CHAT);
  m_btnChat.setHintText(TSTRING(HUB_CHAT_FRAME_TITLE));

  PeersToolbarButton* const buttons[] = { &m_btnAdvice, &m_btnSubscriptions, &m_btnDownloadQueue, &m_btnFinishedDownloads, &m_btnChat };
  for (int i = 0; i < COUNTOF(buttons); ++i) {
    if (buttons[i]->m_hWnd) {
      buttons[i]->setColors(TEXT_COLOR, ORANGE_COLOR, HILITE_COLOR);
      buttons[i]->SetFont(WinUtil::font);
    }
  }
#ifdef _DEBUG
  //m_search.SetWindowText(_T("iso"));
  //PostMessageDelayer::execute(m_searchButton.m_hWnd, BM_CLICK, 2000);
  //PostMessageDelayer::execute(m_btnChat.m_hWnd, BM_CLICK, 1000);
  //PostMessageDelayer::execute(2000, WinUtil::mainWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_SETTINGS,0));
#endif
  return 0;
}

LRESULT PeersToolbar::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CRect r;
  GetClientRect(r);
  // позиционирование верхних 5 кнопок
  PeersToolbarButton* const buttons[] = { &m_btnAdvice, &m_btnSubscriptions, &m_btnDownloadQueue, &m_btnFinishedDownloads, &m_btnChat };
  r.right -= buttons[0]->getPreferredSize().cx / 2;
  r.top = BUTTON_TOP_MARGIN;
  r.bottom = getHeight() - BUTTON_BOTTOM_MARGIN;
  for (int i = COUNTOF(buttons); --i >= 0;) {
    if (buttons[i]->m_hWnd) {
		const int buttonWidth = buttons[i]->getPreferredSize().cx;
		r.left = r.right - buttonWidth;
		buttons[i]->MoveWindow(r);
		r.right = r.left;
		r.right -= BUTTON_H_GAP;
    }
  }
  return 0;
}

LRESULT PeersToolbar::onOpenWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  switch (wID) {
  case IDC_QUEUE: 
    QueueFrame::openWindow(false); 
    break;
  case IDC_UPLOAD_QUEUE: 
    WaitingUsersFrame::openWindow(false); 
    break;
  case IDC_FINISHED_UL:
    FinishedULFrame::openWindow(false); 
    break;
  case IDC_FINISHED:
    FinishedFrame::openWindow(false); 
	break;
  case IDC_ADVICE_WINDOW: 
	  AdviceFrame::openWindow(false); 
	  break;
  case IDC_SUBSCRIPTIONS:
	  SubscriptionFrame::openWindow(false);
	  break;
  }
  return 0;
}

LRESULT CSearchEdit::onKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  if (wParam == VK_RETURN) {
    ::SendMessage(::GetDlgItem(GetParent(), IDC_PEERS_TOOLBAR_SEARCH_BUTTON), BM_CLICK, 0, 0);
  }
  else if (wParam == VK_TAB) {
    ::SetFocus(::GetDlgItem(GetParent(), IDC_PEERS_TOOLBAR_SEARCH_BUTTON));
  }
  else {
    bHandled = FALSE;
  }
  return 0;
}

LRESULT CSearchEdit::onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  if (wParam == VK_RETURN) {
    bHandled = TRUE;
  }
  else if (wParam != VK_TAB) {
    bHandled = FALSE;
  }
  return 0;
}

HWND PeersToolbarButton::Create(ATL::_U_STRINGorID resourceId, ATL::_U_STRINGorID hoverResourceId, HWND hWndParent, _U_MENUorID MenuOrID) {
	if (resourceId.m_lpstr != NULL) {
		if (m_image.LoadBitmap(resourceId)) {
			BITMAP bmp;
			if (m_image.GetBitmap(&bmp)) {
				m_width = bmp.bmWidth;
				m_height = bmp.bmHeight;
			}
		}
		if (hoverResourceId.m_lpstr != NULL) {
			if (m_hoverImage.LoadBitmap(hoverResourceId)) {
				BITMAP bmp;
				if (m_hoverImage.GetBitmap(&bmp)) {
					dcassert(m_width == bmp.bmWidth && m_height == bmp.bmHeight);
				}
			}
		}
	}
	return FlatButton::Create(hWndParent, NULL, MenuOrID);
}

SIZE PeersToolbarButton::getPreferredSize() {
	SIZE sz;
	sz.cx = m_width;
	sz.cy = m_height;
	return sz;
}

void PeersToolbarButton::drawButton(CDCHandle dc, const CRect &r, bool selected) {
	if (m_image) {  
		CDC bitmapDC;
		bitmapDC.CreateCompatibleDC(dc);
		const HBITMAP oldBitmap = bitmapDC.SelectBitmap(isHover() ? m_hoverImage : m_image);
		dc.BitBlt(r.left + (selected ? 1 : 0), r.top + (selected ? 1 : 0), r.Width(), r.Height(), bitmapDC, 0, 0, SRCCOPY);
		bitmapDC.SelectBitmap(oldBitmap);
	}
}

LRESULT PeersToolbarButton::onSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    SetCursor(::LoadCursor(NULL, IDC_HAND));
	return TRUE;
}
