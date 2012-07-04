#include "stdafx.h"
#include "../windows/resource.h"
#include "MagnetPage.h"
#include "../peers/ControlAdjuster.h"

PropPage::TextItem MagnetPage::texts[] = {
	{ IDC_MAGNETPAGE_TITLE, ResourceManager::SETTINGS_MAGNETPAGE },
	{ IDC_MAGNET_ASK_IN_DIALOG, ResourceManager::SETTINGS_MAGNET_ASK_IN_DIALOG },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item MagnetPage::items[] = {
  { IDC_MAGNET_ASK_IN_DIALOG, SettingsManager::MAGNET_ASK, PropPage::T_BOOL_INVERTED },
  { 0, 0, PropPage::T_END }
};

MagnetPage::~MagnetPage() {
}

LRESULT MagnetPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate(texts);
	initHeader(IDC_MAGNETPAGE_TITLE);
	PropPage::read(items);
	return TRUE;
}

void MagnetPage::write() {
	PropPage::write(items);
}
