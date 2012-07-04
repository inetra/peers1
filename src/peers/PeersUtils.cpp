#include "stdafx.h"
#include "PeersUtils.h"
#include "PeersVersion.h"

string PeersUtils::getUserAgent() {
  return APPNAME " v" VERSIONSTRING;
}

const string PeersUtils::PEERS_HUB("peers.cn.ru");
