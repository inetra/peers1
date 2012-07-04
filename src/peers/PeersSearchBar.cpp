#include "stdafx.h"
#include "PeersSearchBar.h"
#include "ControlAdjuster.h"
#include "SearchFrmFactory.h"

#define BACKGROUND_COLOR	HEX_RGB(0xFFE2AB)
#define TEXT_COLOR			RGB(0,0,0)
#define HILITE_COLOR		HEX_RGB(0xF0B648)

#define TOGGLE_BUTTON_WIDTH		125
#define SEARCH_BUTTON_WIDTH		80
#define CONTROL_TOP_MARGIN      3
#define CONTROL_H_GAP           8
#define CONTROL_LEFT_PADDING	8

PeersSearchBar::PeersSearchBar(PeersToolbarListener* listener): 
m_listener(listener),
m_search((LPCTSTR) NULL),
m_expanded(false),
m_compactHeight(35),
m_expandedHeight(70)
{
#ifdef _DEBUG
	m_expanded = true;
#endif
}

LRESULT PeersSearchBar::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_backgroundBrush.CreateSolidBrush(BACKGROUND_COLOR);
	m_search.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE, IDC_PEERS_TOOLBAR_SEARCH_FIELD);
	m_search.SetFont(WinUtil::font);
	m_searchButton.Create(m_hWnd, rcDefault, CTSTRING(PEERS_TOOLBAR_SEARCH_BUTTON), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_PUSHBUTTON, 0, IDC_PEERS_TOOLBAR_SEARCH_BUTTON);
	m_searchButton.SetFont(WinUtil::font);
	createAvancedSearchFields();
	return 0;
}

void PeersSearchBar::createAvancedSearchFields() {
	m_searchPrompt.Create(m_hWnd, rcDefault, CTSTRING(PEERS_TOOLBAR_SEARCH_PROMPT), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_PUSHBUTTON);
	m_searchPrompt.SetFont(WinUtil::font);
	m_searchPrompt.setLeftPadding(CONTROL_H_GAP);
	m_searchPrompt.setBackgroundColor(BACKGROUND_COLOR);
	m_toggleButton.setAlignment(FlatButton::taCenter);
	m_toggleButton.Create(m_hWnd, CTSTRING_I(getButtonText()), IDC_PEERS_TOOLBAR_SEARCH_TOGGLE);
	m_toggleButton.setColors(TEXT_COLOR, BACKGROUND_COLOR, HILITE_COLOR);
	m_toggleButton.SetFont(WinUtil::font);
	typeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	typeLabel.SetFont(WinUtil::systemFont, FALSE);
	typeLabel.SetWindowText(CTSTRING(FILE_TYPE));
	ctrlFiletype.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, WS_EX_CLIENTEDGE, IDC_FILETYPES);
	sizeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	sizeLabel.SetFont(WinUtil::systemFont, FALSE);
	sizeLabel.SetWindowText(CTSTRING(SIZE));
	ctrlMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	ctrlMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlMode.AddString(CTSTRING(SEARCH_SIZE_DONT_CARE));
	ctrlMode.AddString(CTSTRING(AT_LEAST));
	ctrlMode.AddString(CTSTRING(AT_MOST));
	ctrlMode.AddString(CTSTRING(EXACT_SIZE));
	ctrlMode.SetCurSel(1);
	ctrlSize.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE);
	ctrlSize.SetFont(WinUtil::systemFont, FALSE);
	ctrlSizeMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	ctrlSizeMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlSizeMode.AddString(CTSTRING(B));
	ctrlSizeMode.AddString(CTSTRING(KB));
	ctrlSizeMode.AddString(CTSTRING(MB));
	ctrlSizeMode.AddString(CTSTRING(GB));
	ctrlSizeMode.SetCurSel(2);
}

LRESULT PeersSearchBar::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CRect r;
	GetClientRect(r);
	r.left += CONTROL_LEFT_PADDING;
	r.right = r.left + m_searchPrompt.selectWidth();
	m_searchPrompt.MoveWindow(r.left, r.top, r.right, r.top + m_compactHeight);
	// позиционирование поля и кнопки поиска
	const int controlLeft = r.right + CONTROL_H_GAP;
	const SIZE searchSize = ControlAdjuster::adjustEditSize(m_search, 64);
	CRect rectSearch;
	rectSearch.top = (m_compactHeight - searchSize.cy) / 2;
	rectSearch.bottom = rectSearch.top + searchSize.cy;
	rectSearch.left = controlLeft;
	rectSearch.right = rectSearch.left + searchSize.cx;
	m_search.MoveWindow(rectSearch);
	rectSearch.left = rectSearch.right + CONTROL_H_GAP;
	rectSearch.right = rectSearch.left + SEARCH_BUTTON_WIDTH;
	m_searchButton.MoveWindow(rectSearch);
	if (m_toggleButton) {
		CRect br;
		br.left = rectSearch.right + CONTROL_H_GAP;
		br.right = br.left + TOGGLE_BUTTON_WIDTH;
		const int buttonHeight = m_compactHeight * 2 / 3;
		br.top = (m_compactHeight - buttonHeight) / 2;
		br.bottom = br.top + buttonHeight;
		m_toggleButton.MoveWindow(br);
	}

	// позиционирование второй строки контролов
	SIZE sizes[6]; 
	sizes[0] = ControlAdjuster::adjustStaticSize(typeLabel);
	sizes[1] = ControlAdjuster::adjustComboBoxSize(ctrlFiletype);
	sizes[1].cx += ctrlFiletype.getExtraItemWidth();
	sizes[2] = ControlAdjuster::adjustStaticSize(sizeLabel);
	sizes[3] = ControlAdjuster::adjustComboBoxSize(ctrlMode);
	sizes[4] = ControlAdjuster::adjustEditSize(ctrlSize, 6);
	sizes[5] = ControlAdjuster::adjustComboBoxSize(ctrlSizeMode);
	static const bool maxHeight[6] = { false, true, false, true, true, true };
	const int borderYdelta = GetSystemMetrics(SM_CYBORDER) * 6;
	const int comboBoxHeights[6] = {
		0,
		ctrlFiletype.calculateItemHeight() + borderYdelta,
		0,
		ControlAdjuster::getComboBoxHeight(ctrlMode),
		0,
		ControlAdjuster::getComboBoxHeight(ctrlSizeMode)
	};
	HWND windows[6] = { typeLabel, ctrlFiletype, sizeLabel, ctrlMode, ctrlSize, ctrlSizeMode };
	int controlMaxHeight = 0;
	for (int i = 0; i < 6; ++i) {
		if (maxHeight[i]) {
			const int controlHeight = comboBoxHeights[i] ? comboBoxHeights[i] : sizes[i].cy;
			if (controlHeight > controlMaxHeight) {
				controlMaxHeight = controlHeight;
			}
		}
	}
	// dcdebug("controlMaxHeight=%d\n", controlMaxHeight);
	const int top = m_compactHeight + CONTROL_TOP_MARGIN;
	int left = controlLeft;
	for (int i = 0; i < 6; ++i) {
		int controlTop = top;
		if (maxHeight[i]) {
			::MoveWindow(windows[i], left, controlTop, sizes[i].cx, comboBoxHeights[i] ? sizes[i].cy : controlMaxHeight, TRUE);
			if (comboBoxHeights[i]) {
				::SendMessage(windows[i], CB_SETITEMHEIGHT, (WPARAM) -1, controlMaxHeight - borderYdelta);
			}
		}
		else {
			controlTop += (controlMaxHeight - sizes[i].cy) / 2;
			::MoveWindow(windows[i], left, controlTop, sizes[i].cx, sizes[i].cy, TRUE);
		}
		left += sizes[i].cx + CONTROL_H_GAP;
	}
	return 0;
}

LRESULT PeersSearchBar::onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return 1;
}

LRESULT PeersSearchBar::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CPaintDC dc(m_hWnd);
	CRect r;
	GetClientRect(r);
	dc.FillRect(r, m_backgroundBrush);
	return 0;
}

HWND PeersSearchBar::Create(HWND hWndParent) {
	return CWindowImpl<PeersSearchBar>::Create(hWndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

const ResourceManager::Strings PeersSearchBar::getButtonText() const {
	return !m_expanded ? ResourceManager::PEERS_TOOLBAR_EXPAND : ResourceManager::PEERS_TOOLBAR_COLLAPSE;
}

LRESULT PeersSearchBar::onCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CDCHandle dc((HDC) wParam);
	dc.SetBkColor(BACKGROUND_COLOR);
	dc.SetTextColor(TEXT_COLOR);
	return (LRESULT) (HBRUSH) m_backgroundBrush;
}

LRESULT PeersSearchBar::onCtrlColorEdit(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CDCHandle dc((HDC) wParam);
	dc.SetBkColor(GetSysColor(COLOR_WINDOW));
	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	return (LRESULT) GetSysColorBrush(COLOR_WINDOW);
}

LRESULT PeersSearchBar::onToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	toggleToolbar();
	return 0;
}

void PeersSearchBar::toggleToolbar() {
	m_expanded = !m_expanded;
	m_toggleButton.SetWindowText(CTSTRING_I(getButtonText()));
	if (m_listener != NULL) {
		m_listener->onToolbarSearchToggle();
	}
}

static int sizeScales[] = {
  1,
  1024,
  1024 * 1024,
  1024 * 1024 * 1024
};

LRESULT PeersSearchBar::onSearchButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  const int len = m_search.GetWindowTextLength();
  if (len > 0) {
    AutoArray<TCHAR> searchText(len + 1);
    m_search.GetWindowText(searchText, len + 1);
    SearchFrameFactory::Request request;
	request.str = Util::trim(tstring(searchText));
	if (request.str.empty()) {
		return 0;
	}
	if (BOOLSETTING(CLEAR_SEARCH)) {
		m_search.SetWindowText(_T(""));
	}
	if (WinUtil::handleLink(request.str)) {
		return 0;
	}
    if (m_expanded) {
      request.type = (SearchManager::TypeModes) ctrlFiletype.GetCurSel();
      const int sizeLen = ctrlSize.GetWindowTextLength();
      AutoArray<TCHAR> size(sizeLen + 1);
      ctrlSize.GetWindowText(size, sizeLen + 1);
      TCHAR* comma = wcschr(size, _T(','));
      if (comma) {
        *comma = '.';
      }
      double lsize = _wtof(size);
      const int scale = ctrlSizeMode.GetCurSel();
      if (scale >= 0 && scale < COUNTOF(sizeScales)) {
        lsize *= sizeScales[scale];
      }
      request.size = (int64_t) lsize;
      request.mode = (request.size > 0) ? (SearchManager::SizeModes) ctrlMode.GetCurSel() : SearchManager::SIZE_DONTCARE;
    }
    SearchFrameFactory::openWindow(request);
  }
  return 0;
}

void PeersSearchBar::setFocusToSearch() {
  m_search.SetFocus();
}

void PeersSearchBar::setSearchText(const tstring& s) {
  m_search.SetWindowText(s.c_str());
}

void PeersSearchBar::setSearchFileType(SearchManager::TypeModes fileType) {
  ctrlFiletype.SetCurSel(fileType);
}

void PeersSearchBar::setSearchSize(SearchManager::SizeModes sizeMode, int64_t fileSize) {
  ctrlMode.SetCurSel(sizeMode);
  if (sizeMode == SearchManager::SIZE_DONTCARE) {
    ctrlSize.SetWindowText(_T(""));
    ctrlSizeMode.SetCurSel(2);
  }
  else {
    for (int i = COUNTOF(sizeScales); --i >= 0;) {
      if (fileSize % sizeScales[i] == 0) {
        ctrlSize.SetWindowText(Util::toStringW(fileSize / sizeScales[i]).c_str());
        ctrlSizeMode.SetCurSel(i);
        return;
      }
    }
    ctrlSizeMode.SetCurSel(0);
    ctrlSize.SetWindowText(Util::toStringW(fileSize).c_str());
  }
}

void PeersSearchBar::expand() {
  if (!m_expanded) {
    toggleToolbar();
  }
}

void PeersSearchBar::collapse() {
  if (m_expanded) {
    toggleToolbar();
  }
}

LRESULT PeersSearchPrompt::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CPaintDC dc(m_hWnd);
	CRect r;
	GetClientRect(r);
	CBrush brush;
	brush.CreateSolidBrush(m_backgroundColor);
	dc.FillRect(r, brush);
	m_icon.draw(dc, r.left, (r.Height() - m_icon.getHeight()) / 2);
	const int len = GetWindowTextLength();
	if (len > 0) {
		r.left += m_leftPadding + m_icon.getWidth();
		AutoArray<TCHAR> caption(len + 1);
		GetWindowText(caption, len + 1);
		const HFONT oldFont = dc.SelectFont(WinUtil::boldFont);
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(TEXT_COLOR);
		dc.DrawText(caption, len, r, DT_SINGLELINE | DT_VCENTER);
		dc.SelectFont(oldFont);
	}
	return 0;
}

int PeersSearchPrompt::selectWidth() const {
	const int len = GetWindowTextLength();
	if (len > 0) {
		AutoArray<TCHAR> caption(len + 1);
		GetWindowText(caption, len + 1);
		CRect r;
		GetClientRect(r);
		CClientDC dc(m_hWnd);
		const HFONT oldFont = dc.SelectFont(WinUtil::boldFont);
		dc.DrawText(caption, len, r, DT_SINGLELINE | DT_CALCRECT);
		dc.SelectFont(oldFont);
		return r.Width() + m_icon.getHeight() + m_leftPadding;
	}
	else {
		return 1;
	}
}
