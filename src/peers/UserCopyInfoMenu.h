#pragma once
#include "../windows/OMenu.h"
#include "../windows/UserInfo.h"

class ATL_NO_VTABLE UserInfoGetter {
public:
  virtual UserInfo* getSelectedUser() = 0;
};

class UserCopyInfoMenu : public CMessageMap {
private:
  UserInfoGetter* m_userInfoGetter;
public:
  UserCopyInfoMenu(UserInfoGetter* userInfoGetter): m_userInfoGetter(userInfoGetter) {
  }
  HMENU build();
  tstring copyInfo(UserInfo* ui, int commandId) const;

  BEGIN_MSG_MAP(UserCopyInfoMenu)
    COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_EXACT_SHARE, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_DESCRIPTION, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_TAG, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_EMAIL_ADDRESS, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_IP, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_NICK_IP, onCopyUserInfo)
    COMMAND_ID_HANDLER(IDC_COPY_ALL, onCopyUserInfo)
  END_MSG_MAP()

  LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
