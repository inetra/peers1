#include "stdafx.h"
#include "SearchSettingsPage.h"

PropPage::TextItem SearchSettingsPage::texts[] = {
  { IDC_SEARCH_SETTINGS_TITLE, ResourceManager::SETTINGS_SEARCH },
  { 0, ResourceManager::LAST }
};

PropPage::ListItem SearchSettingsPage::listItems[] = {
  { SettingsManager::CLEAR_SEARCH, ResourceManager::SETTINGS_CLEAR_SEARCH },
  { SettingsManager::SEARCH_EXPAND_TOOLBAR_ON_ACTIVATE, ResourceManager::SETTINGS_SEARCH_EXPAND_TOOLBAR_ON_ACTIVATE },
  { SettingsManager::SEARCH_RESTORE_CONDITION_ON_ACTIVATE, ResourceManager::SEARCH_RESTORE_CONDITION_ON_ACTIVATE },
  { 0, ResourceManager::LAST }
};

void SearchSettingsPage::write() {
  PropPage::writeCheckBoxList(listItems, GetDlgItem(IDC_SEARCH_OPTIONS));
}

LRESULT SearchSettingsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  PropPage::translate(texts);
  PropPage::initHeader(IDC_SEARCH_SETTINGS_TITLE);
  PropPage::readCheckBoxList(listItems, GetDlgItem(IDC_SEARCH_OPTIONS));
  return 0;
}
