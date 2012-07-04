/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(GENERAL_PAGE_H)
#define GENERAL_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "../peers/FormattedText.h"
#include "../client/peers/IpBlocksLoader.h"
#include "../peers/CaptionFont.h"

class RadioButtonList : public CWindowImpl<RadioButtonList> {
private:
	vector<CButton> providerButtons;
	void GetScrollInfo(SCROLLINFO& si);
	void setScrollPos(int newPos);
	int adjustNewPos(int newPos, const SCROLLINFO& si) {
		if (newPos < si.nMin) {
			newPos = si.nMin;
		}
		if (newPos > si.nMax - (int) si.nPage + 1) {
			newPos = si.nMax - si.nPage + 1;
		}
		return newPos;
	}
public:
	void addButton(const CButton& b) {
		providerButtons.push_back(b);
	}

	void layoutButtons(int radioButtonHeight, int offset);

	void setupScrollbar(int radioButtonHeight);

	bool isProviderSelected();

	int getSelectedProvider();

	BEGIN_MSG_MAP(RadioButtonList)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
		MESSAGE_HANDLER(WM_PAINT, onPaint);
		MESSAGE_HANDLER(WM_VSCROLL, onVScroll);
		MESSAGE_HANDLER(WM_MOUSEWHEEL, onMouseWheel);
	END_MSG_MAP();

	LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }

	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
		CRect r;
		GetClientRect(r);
		CBrush brush;
		brush.CreateSysColorBrush(COLOR_3DFACE);
		dc.FillRect(r, brush);
		return 0;
	}

	LRESULT onVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	int getButtonHeight() {	
		return WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) * 3 / 2;
	}

};

class GeneralPage : 
  public PeersPropertyPage<IDD_GENERALPAGE>,
  public PropPageImpl<GeneralPage,0,false>
{
public:
  GeneralPage()
#ifdef INCLUDE_PROVIDE_SELECTION
: m_providerNoticeFont(CaptionFont::NORMAL, 9, 8) 
#endif
  {
    SetTitle(CTSTRING(SETTINGS_GENERAL));
  }
  ~GeneralPage() { ConnTypes.Destroy(); }

  BEGIN_MSG_MAP(GeneralPage)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
    MESSAGE_HANDLER(WM_SIZE, onSize)
    COMMAND_HANDLER(IDC_NICK, EN_CHANGE, onTextChanged)
    COMMAND_HANDLER(IDC_EMAIL, EN_CHANGE, onTextChanged)
    COMMAND_HANDLER(IDC_DESCRIPTION, EN_CHANGE, onTextChanged)
    COMMAND_ID_HANDLER(IDC_BW_BOTH, onClickedRadioButton)
    COMMAND_ID_HANDLER(IDC_BW_SIMPLE, onClickedRadioButton)
    COMMAND_ID_HANDLER(IDC_SHOW_SPEED_CHECK, onClickedRadioButton)
  END_MSG_MAP()

  LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onGetIP(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
  LRESULT onTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onClickedRadioButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  virtual void write();
  virtual void validation();

private:
  int getCurrentProvider(SettingsManager::IntSetting setting);

  static Item items[];
  static TextItem texts[];
  CComboBoxEx ctrlConnectionType;
  CComboBox ctrlConnection;	
  CComboBox ctrlDownloadSpeed, ctrlUploadSpeed;
  CImageList ConnTypes;
#ifdef INCLUDE_PROVIDE_SELECTION
  FormattedText m_providerNotice;
  void updateProviderNoticeLayout();
  auto_ptr<IpBlocksLoader> providers;
  RadioButtonList providerList;
  CaptionFont m_providerNoticeFont;
#endif
  void fixControls();
};

#endif // !defined(GENERAL_PAGE_H)

/**
 * @file
 * $Id: GeneralPage.h,v 1.11.2.1 2008/06/18 01:56:45 alexey Exp $
 */
