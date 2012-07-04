/*
 * ApexDC speedmod (c) SMT 2007
 */
#include "stdafx.h"
#include "MessagesPage.h"

PropPage::TextItem MessagesPage::texts[] = {
        { IDC_SETTINGS_DEFAULT_AWAY_MSG, ResourceManager::SETTINGS_DEFAULT_AWAY_MSG },
        { IDC_PROTECT_PRIVATE, ResourceManager::SETTINGS_PROTECT_PRIVATE },
        { IDC_SETTINGS_PASSWORD, ResourceManager::SETTINGS_PASSWORD },
        { IDC_SETTINGS_PASSWORD_HINT, ResourceManager::SETTINGS_PASSWORD_HINT },
        { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item MessagesPage::items[] = {
        { IDC_DEFAULT_AWAY_MESSAGE, SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR },
        { IDC_PROTECT_PRIVATE, SettingsManager::PROTECT_PRIVATE, PropPage::T_BOOL },
        { IDC_PASSWORD, SettingsManager::PM_PASSWORD, PropPage::T_STR },
        { IDC_PASSWORD_HINT, SettingsManager::PM_PASSWORD_HINT, PropPage::T_STR },
        { IDC_SLOTASK_STR, SettingsManager::SLOT_ASK, PropPage::T_STR },
        { 0, 0, PropPage::T_END }
};

MessagesPage::ListItem MessagesPage::listItems[] = {
        { SettingsManager::SEND_SLOTGRANT_MSG, ResourceManager::SEND_SLOTGRANT_MSG },
        { SettingsManager::NO_AWAYMSG_TO_BOTS, ResourceManager::SETTINGS_NO_AWAYMSG_TO_BOTS },
        { SettingsManager::USE_CTRL_FOR_LINE_HISTORY, ResourceManager::SETTINGS_USE_CTRL_FOR_LINE_HISTORY },
//[-]PPA        { SettingsManager::NO_EMOTES_LINKS, ResourceManager::NO_EMOTES_LINKS },
        { SettingsManager::TIME_STAMPS, ResourceManager::SETTINGS_TIME_STAMPS },
        { SettingsManager::STATUS_IN_CHAT, ResourceManager::SETTINGS_STATUS_IN_CHAT },
        { SettingsManager::SHOW_JOINS, ResourceManager::SETTINGS_SHOW_JOINS },
        { SettingsManager::FAV_SHOW_JOINS, ResourceManager::SETTINGS_FAV_SHOW_JOINS },
        { SettingsManager::SUPPRESS_MAIN_CHAT, ResourceManager::SETTINGS_ADVANCED_SUPPRESS_MAIN_CHAT },
        { SettingsManager::FORMAT_BIU, ResourceManager::FORMAT_BIU },
        { SettingsManager::MULTILINE_CHAT_INPUT, ResourceManager::MULTILINE_CHAT_INPUT },
//[-]PPA        { SettingsManager::CZCHARS_DISABLE, ResourceManager::SETCZDC_CZCHARS_DISABLE },

        { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT MessagesPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
        PropPage::read(items);
        PropPage::readCheckBoxList(listItems, GetDlgItem(IDC_MESSAGES_BOOLEANS));
        PropPage::translate(texts);
        // Do specialized reading here
        return TRUE;
}

void MessagesPage::write() {
        PropPage::write(items);
        PropPage::writeCheckBoxList(listItems, GetDlgItem(IDC_MESSAGES_BOOLEANS));
        // Do specialized writing here
}

