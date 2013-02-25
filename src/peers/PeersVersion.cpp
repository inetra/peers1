#include "stdafx.h"
#include "PeersVersion.h"

TCHAR HOMEPAGE[] = _T("http://www.cn.ru/peers");
#ifdef _DEBUG
char VERSIONFILE[] = "http://127.0.0.1/peers/version.xml";
#if 0
char NETWORK_SETUP_SERVICE_URL[] = "http://127.0.0.1/peers/network.xml";
#endif
char NETWORK_SETUP_SERVICE_URL[] = "http://peersdata.cn.ru/network/";
#else
char VERSIONFILE[] = "http://firmware.cn.ru/firmware/peers1/version.xml";
char NETWORK_SETUP_SERVICE_URL[] = "http://peersdata.cn.ru/network/";
#endif
TCHAR HELP_REDIRECT_URL[] = _T("http://www.cn.ru/peers?link=");
char ADVICE_CONTENT[] = "http://static.cn.ru/peers/static?noadv";
char NANONET_NUMBER_SERVICE_URL[] = "http://www.cn.ru/mansk/nano/?nick=";

char PINGER_ADDRESS[] = "http://analytics.cn.ru/nonexistent.php";