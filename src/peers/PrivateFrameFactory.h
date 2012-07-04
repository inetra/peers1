#pragma once
#include "../client/User.h"

class PrivateFrameFactory {
public:
  static void gotMessage(Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, bool annoying); // !SMT!-S
  static void openWindow(const UserPtr& replyTo, const tstring& aMessage = Util::emptyStringT);
  static bool isOpen(const UserPtr& u);
  static bool closeUser(const UserPtr& u); // !SMT!-S
  static void closeAll();
  static void closeAllOffline();
};
