#include "stdafx.h"

#include "LimitPage.h"

PropPage::TextItem LimitPage::texts[] = {
	{ IDC_THROTTLE_ENABLE, ResourceManager::SETCZDC_ENABLE_LIMITING },
	{ IDC_CZDC_TRANSFER_LIMITING, ResourceManager::SETCZDC_TRANSFER_LIMITING },
	{ IDC_CZDC_UP_SPEEED1, ResourceManager::SETCZDC_UPLOAD_SPEED },
	{ IDC_SETTINGS_KBPS1, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS2, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS3, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS4, ResourceManager::KBPS },
	{ IDC_CZDC_DW_SPEEED1, ResourceManager::SETCZDC_DOWNLOAD_SPEED },
	{ IDC_TIME_LIMITING, ResourceManager::SETCZDC_ALTERNATE_LIMITING },
	{ IDC_CZDC_TO, ResourceManager::SETCZDC_TO },
	{ IDC_CZDC_SLOW_DISCONNECT, ResourceManager::SETCZDC_SLOW_DISCONNECT },
	{ IDC_DISCONNECTING_ENABLE, ResourceManager::SETCZDC_DISCONNECTING_ENABLE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

PropPage::Item LimitPage::items[] = {
	{ IDC_MX_UP_SP_LMT_NORMAL, SettingsManager::MAX_UPLOAD_SPEED_LIMIT_NORMAL, PropPage::T_INT },
	{ IDC_MX_DW_SP_LMT_NORMAL, SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, PropPage::T_INT },
	{ IDC_TIME_LIMITING, SettingsManager::TIME_DEPENDENT_THROTTLE, PropPage::T_BOOL },
	{ IDC_MX_UP_SP_LMT_TIME, SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, PropPage::T_INT },
	{ IDC_MX_DW_SP_LMT_TIME, SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, PropPage::T_INT },
	{ IDC_BW_START_TIME, SettingsManager::BANDWIDTH_LIMIT_START, PropPage::T_INT },
	{ IDC_BW_END_TIME, SettingsManager::BANDWIDTH_LIMIT_END, PropPage::T_INT },
	{ IDC_THROTTLE_ENABLE, SettingsManager::THROTTLE_ENABLE, PropPage::T_BOOL },
	{ IDC_DISCONNECTING_ENABLE, SettingsManager::DISCONNECTING_ENABLE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT LimitPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  // always on for now
  ::EnableWindow(GetDlgItem(IDC_DISCONNECTING_ENABLE), false); 
  PropPage::translate(texts);
  PropPage::read(items);

  CUpDownCtrl(GetDlgItem(IDC_UPLOADSPEEDSPIN)).SetRange32(0, 99999);
  CUpDownCtrl(GetDlgItem(IDC_DOWNLOADSPEEDSPIN)).SetRange32(0, 99999);
  CUpDownCtrl(GetDlgItem(IDC_UPLOADSPEEDSPIN_TIME)).SetRange32(0, 99999);
  CUpDownCtrl(GetDlgItem(IDC_DOWNLOADSPEEDSPIN_TIME)).SetRange32(0, 99999);

  timeCtrlBegin.Attach(GetDlgItem(IDC_BW_START_TIME));
  timeCtrlEnd.Attach(GetDlgItem(IDC_BW_END_TIME));

  timeCtrlBegin.AddString(CTSTRING(MIDNIGHT));
  timeCtrlEnd.AddString(CTSTRING(MIDNIGHT));
  for (int i = 1; i < 12; ++i) {
    timeCtrlBegin.AddString((Util::toStringW(i) + CTSTRING(AM)).c_str());
    timeCtrlEnd.AddString((Util::toStringW(i) + CTSTRING(AM)).c_str());
  }
  timeCtrlBegin.AddString(CTSTRING(NOON));
  timeCtrlEnd.AddString(CTSTRING(NOON));
  for (int i = 1; i < 12; ++i) {
    timeCtrlBegin.AddString((Util::toStringW(i) + CTSTRING(PM)).c_str());
    timeCtrlEnd.AddString((Util::toStringW(i) + CTSTRING(PM)).c_str());
  }

  timeCtrlBegin.SetCurSel(SETTING(BANDWIDTH_LIMIT_START));
  timeCtrlEnd.SetCurSel(SETTING(BANDWIDTH_LIMIT_END));

  timeCtrlBegin.Detach();
  timeCtrlEnd.Detach();

  fixControls();

  // Do specialized reading here

  return TRUE;
}

void LimitPage::write()
{
	PropPage::write(items);

/*	if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) > 0) {
		if( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) < ((2 * SETTING(SLOTS)) + 3) ) {
			settings->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_NORMAL, ((2 * SETTING(SLOTS)) + 3) );
		}
		if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL) == 0) ) {
			settings->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, (SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL)*7) );
		}
	}

	if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) > 0) {
		if( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < ((2 * SETTING(SLOTS)) + 3) ) {
			settings->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, ((2 * SETTING(SLOTS)) + 3) );
		}
		if ( (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > ( SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) * 7)) || ( SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0) ) {
			settings->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, (SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)*7) );
		}
	}*/

	// Do specialized writing here

	timeCtrlBegin.Attach(GetDlgItem(IDC_BW_START_TIME));
	timeCtrlEnd.Attach(GetDlgItem(IDC_BW_END_TIME));
	settings->set(SettingsManager::BANDWIDTH_LIMIT_START, timeCtrlBegin.GetCurSel());
	settings->set(SettingsManager::BANDWIDTH_LIMIT_END, timeCtrlEnd.GetCurSel());
	timeCtrlBegin.Detach();
	timeCtrlEnd.Detach(); 
}

void LimitPage::fixControls() {
	bool state = (IsDlgButtonChecked(IDC_THROTTLE_ENABLE) != 0);
	::EnableWindow(GetDlgItem(IDC_MX_UP_SP_LMT_NORMAL), state);
	::EnableWindow(GetDlgItem(IDC_UPLOADSPEEDSPIN), state);
	::EnableWindow(GetDlgItem(IDC_MX_DW_SP_LMT_NORMAL), state);
	::EnableWindow(GetDlgItem(IDC_DOWNLOADSPEEDSPIN), state);
	::EnableWindow(GetDlgItem(IDC_TIME_LIMITING), state);

	state = ((IsDlgButtonChecked(IDC_THROTTLE_ENABLE) != 0) && (IsDlgButtonChecked(IDC_TIME_LIMITING) != 0));
	::EnableWindow(GetDlgItem(IDC_BW_START_TIME), state);
	::EnableWindow(GetDlgItem(IDC_BW_END_TIME), state);
	::EnableWindow(GetDlgItem(IDC_MX_UP_SP_LMT_TIME), state);
	::EnableWindow(GetDlgItem(IDC_UPLOADSPEEDSPIN_TIME), state);
	::EnableWindow(GetDlgItem(IDC_MX_DW_SP_LMT_TIME), state);
	::EnableWindow(GetDlgItem(IDC_DOWNLOADSPEEDSPIN_TIME), state);
}

LRESULT LimitPage::onChangeCont(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch (wID) {
	case IDC_TIME_LIMITING:
	case IDC_THROTTLE_ENABLE:
		fixControls();
		break;
	}
	return true;
}
