#include "stdafx.h"
#include "AutodetectNetwork.h"
#include "PeersUtils.h"
#include "../logger/Logger.h"
#include "../logger/FileAppender.h"
#include "../utils/SystemUtils.h"

static Logger::Category AUTODETECT("AUTODETECT");

static void registerLogger() {
	if (Logger::Repository::hasAppender(AUTODETECT)) {
		return;
	}
	tstring logPath = SystemUtils::getAppDataFolder() + _T("autodetect.log");
	char path[1024];
	int pathLen = WideCharToMultiByte(CP_ACP, 0, logPath.c_str(), (int) logPath.length(), path, (int) COUNTOF(path), NULL, NULL);
	Logger::Repository::registerAppender(AUTODETECT, new Logger::FileAppender<CriticalSection>(string(path, pathLen), "at"));
}

void DetectNetworkSetupThread::run() {
	Pointer<DetectNetworkSetupThread> thread = new DetectNetworkSetupThread();
	thread->wait();
}

void DetectNetworkSetupThread::on(Failed, HttpConnection*, const string& message) throw() {
	error_log(AUTODETECT, "Request error %s", message.c_str());
	finalize();
}

void DetectNetworkSetupThread::on(Complete, HttpConnection*, const string&) throw() {
	debug_log(AUTODETECT, "parsing response {%s}", m_buffer.c_str());
	try {
		SimpleXML xml;
		xml.fromXML(m_buffer);
		if (xml.findChild("response")) {
			debug_log(AUTODETECT, "found <response>");
			xml.stepIn();
			if (xml.findChild("mode")) {
				string mode = xml.getChildData();
				info_log(AUTODETECT, "mode={%s}", mode.c_str());
				xml.resetCurrentChild();
				string address;
				if (xml.findChild("address")) {
					string address = xml.getChildData();
					info_log(AUTODETECT, "address={%s}", address.c_str());
					if (!address.empty()) {
						SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, address);
					}
				}
				else {
					warn_log(AUTODETECT, "<address> not found");
				}
				debug_log(AUTODETECT, "local address={%s}", connection.m_localIp.c_str());
				if (Util::stricmp(mode, "active") == 0) {
					SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_NAT);
					debug_log(AUTODETECT, "setting ACTIVE mode");
				}
				else if (Util::stricmp(mode, "passive") == 0) {
					SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
				}
				else {
					throw Exception("Wrong mode '" + mode + "'");
				}
			}
			else {
				warn_log(AUTODETECT, "<mode> not found");
			}
		}
		else {
			warn_log(AUTODETECT, "<response> not found");
		}
	} 
	catch (const Exception &e) {
		error_log(AUTODETECT, "Error parsing response: %s", e.getError().c_str());
	}
	finalize();
}

DetectNetworkSetupThread::DetectNetworkSetupThread(): m_done(false), connection(PeersUtils::getUserAgent()) {
	inc();
	registerLogger();
	debug_log(AUTODETECT, "*** Start ***");
	// окно должно создаваться в главном потоке, где есть цикл обработки сообщений
	m_invoker = new InvokeLater(this);
	connection.addListener(this);
	vector<string> addresses;
	Util::getLocalIp(addresses);
	if (!addresses.empty()) {
		string request = string(NETWORK_SETUP_SERVICE_URL);
		request += "?portTCP=";
		request += Util::toString(SETTING(TCP_PORT));
		request += "&portUDP=";
		request += Util::toString(SETTING(UDP_PORT));
		info_log(AUTODETECT, "sending request {%s}", request.c_str());
		connection.downloadFile(request);
	}
}

void DetectNetworkSetupThread::wait() {
	for (int i = 0; i < 20; ++i) {
		if (isDone()) {
			debug_log(AUTODETECT, "*** Completed");
			return;
		}
		Sleep(100);
	}
	warn_log(AUTODETECT, "*** Not completed in expected time");
}
