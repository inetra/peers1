#include "stdafx.h"
#include "UserCopyInfoMenu.h"

HMENU UserCopyInfoMenu::build() {
  CMenu copyMenu;
  copyMenu.CreatePopupMenu();
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
  //copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SHARE, CTSTRING(COPY_EXACT_SHARE));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_DESCRIPTION, CTSTRING(COPY_DESCRIPTION));
  //copyMenu.AppendMenu(MF_STRING, IDC_COPY_TAG, CTSTRING(COPY_TAG));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_EMAIL_ADDRESS, CTSTRING(COPY_EMAIL_ADDRESS));
  //copyMenu.AppendMenu(MF_STRING, IDC_COPY_IP, CTSTRING(COPY_IP));
  //copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK_IP, CTSTRING(COPY_NICK_IP));
  copyMenu.AppendMenu(MF_STRING, IDC_COPY_ALL, CTSTRING(COPY_ALL));
  return copyMenu.Detach();
}

LRESULT UserCopyInfoMenu::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (m_userInfoGetter) {
    UserInfo* ui = m_userInfoGetter->getSelectedUser();
    if (ui) {
      const tstring result = copyInfo(ui, wID);
      if (!result.empty()) {
        WinUtil::setClipboard(result);
      }
    }
  }
  return 0;
}

tstring UserCopyInfoMenu::copyInfo(UserInfo* ui, int commandId) const {
  dcassert(ui);
  switch (commandId) 
  {
  case IDC_COPY_NICK:
    return Text::toT(ui->getNick());
  case IDC_COPY_EXACT_SHARE:
    return Util::formatExactSize(ui->getIdentity().getBytesShared());
  case IDC_COPY_DESCRIPTION:
    return Text::toT(ui->getIdentity().getDescription());
  case IDC_COPY_TAG:
    return Text::toT(ui->getIdentity().getTag());
  case IDC_COPY_EMAIL_ADDRESS:
    return Text::toT(ui->getIdentity().getEmail());
  case IDC_COPY_IP:
    return Text::toT(ui->getIdentity().getIp());
  case IDC_COPY_NICK_IP:
    return 
      _T("User Info:\r\n")
      _T("\tNick: ") + Text::toT(ui->getIdentity().getNick()) + _T("\r\n") +
      _T("\tIP: ") +  Text::toT(ui->getIdentity().getIp()) + _T("\r\n");
  case IDC_COPY_ALL:
    return 
      _T("User Info:\r\n")
      _T("\tNick: ") + Text::toT(ui->getIdentity().getNick()) + _T("\r\n") +
      _T("\tShare: ") + Util::formatBytesW(ui->getIdentity().getBytesShared()) + _T("\r\n") +
      _T("\tDescription: ") + Text::toT(ui->getIdentity().getDescription()) + _T("\r\n") +
      _T("\tTag: ") + Text::toT(ui->getIdentity().getTag()) + _T("\r\n") +
      _T("\tConnection: ") + Text::toT(ui->getIdentity().getConnection()) + _T("\r\n") +
      _T("\tE-Mail: ") + Text::toT(ui->getIdentity().getEmail()) + _T("\r\n") +
      _T("\tClient: ") + Text::toT(ui->getIdentity().get("CT")) + _T("\r\n") +
      _T("\tVersion: ") + Text::toT(ui->getIdentity().get("VE")) + _T("\r\n") +
      //[-]PPA                                        _T("\tMode: ") + ui->getText(COLUMN_MODE) + _T("\r\n") +
      _T("\tHubs: ") + ui->getText(COLUMN_HUBS) + _T("\r\n") +
      _T("\tSlots: ") + ui->getText(COLUMN_SLOTS) + _T("\r\n") +
      //[-]PPA                                        _T("\tUpLimit: ") + ui->getText(COLUMN_UPLOAD_SPEED) + _T("\r\n") +
      _T("\tIP: ") + Text::toT(ui->getIdentity().getIp()) + _T("\r\n") +
      _T("\tPk String: ") + Text::toT(ui->getIdentity().get("PK")) + _T("\r\n") +
      _T("\tLock: " )+ Text::toT(ui->getIdentity().get("LO")) + _T("\r\n")+
      _T("\tSupports: ") + Text::toT(ui->getIdentity().get("SU"));
  default:
    return Util::emptyStringT;
  }
}
