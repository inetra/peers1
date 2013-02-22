/* 
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "ConnectionManager.h"
#include "DownloadManager.h"
#include "UploadManager.h"
#include "CryptoManager.h"
#include "ShareManager.h"
#include "SearchManager.h"
#include "QueueManager.h"
//[-]PPA [Doxygen 1.5.1] #include "ClientManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "FavoriteManager.h"
#include "SettingsManager.h"
#include "FinishedManager.h"
#include "ADLSearch.h"

#include "StringTokenizer.h"

#include "DebugManager.h"
#include "ClientProfileManager.h"
#include "WebServerManager.h"
#include "IgnoreManager.h"
#include "HistoryManager.h"
#include "PGLoader.h"
#include "peers/ClientLifeCycle.h"
#include "peers/ConfigurationPatcher.h"
#include "QoSManager.h"

/*
#ifdef _STLP_DEBUG
void __stl_debug_terminate() {
	int* x = 0;
	*x = 0;
}
#endif
*/

void startup(ProgressCallback* callback, const StartupConfiguration* configuration) {
  // "Dedicated to the near-memory of Nev. Let's start remembering people while they're still alive."
  // Nev's great contribution to dc++
  while(1) break;

  Util::initialize();

  // TODO: copy configuration files from Program Files/Peers/ to AppData/Peers/

  ResourceManager::newInstance();
  SettingsManager::newInstance(configuration->m_settingsManagerVersion);

  LogManager::newInstance();
  TimerManager::newInstance();
  HashManager::newInstance();
  CryptoManager::newInstance();
  SearchManager::newInstance(configuration->m_mainHub);
  ClientManager::newInstance(configuration->m_clientId);
  ConnectionManager::newInstance();
  DownloadManager::newInstance();
  UploadManager::newInstance();
  ShareManager::newInstance();
  FavoriteManager::newInstance();
  QueueManager::newInstance(configuration->m_queueManagerVersion);
  FinishedManager::newInstance();
  ADLSearchManager::newInstance();
  DebugManager::newInstance();
  ClientProfileManager::newInstance();	
  IgnoreManager::newInstance();
  HistoryManager::newInstance();
  QoSManager::newInstance();

  callback->showMessage(_T("Русский язык"));
  ResourceManager::getInstance()->loadLanguage(Util::getDataPath() + "Russian.xml"); // SETTING(LANGUAGE_FILE)

  callback->showMessage(TSTRING(SETTINGS));
  SettingsManager::getInstance()->load();
  ConfigurationPatcher::load();
  // allow localized defaults in string settings
  SettingsManager::getInstance()->setDefaults();

  FavoriteManager::getInstance()->load(dynamic_cast<const FavoriteManagerInitializer*>(configuration));
  CryptoManager::getInstance()->loadCertificates();
  ClientProfileManager::getInstance()->load();	
  UploadManager::getInstance()->load(); 
  WebServerManager::newInstance();
#ifdef PPA_INCLUDE_PG
  if(Util::fileExists(Util::getDataPath() + "PeerGuardian.dll") && Util::fileExists(Util::getDataPath() + "stlport_vc7146.dll")) {
    callback->showMessage(TSTRING(PG_PLUGIN));
  }
#endif
  PGLoader::newInstance();
  callback->showMessage(TSTRING(HASH_DATABASE));
  HashManager::getInstance()->startup();
  callback->showMessage(TSTRING(SHARED_FILES));
  ShareManager::getInstance()->shareDownloads();
  ShareManager::getInstance()->refresh(true, false, true);
  callback->showMessage(TSTRING(DOWNLOAD_QUEUE));
  QueueManager::getInstance()->loadQueue();
}

void shutdown(bool exp /*= false*/) {
  TimerManager::getInstance()->shutdown();
  HashManager::getInstance()->shutdown();
  ConnectionManager::getInstance()->shutdown();

#ifdef PPA_INCLUDE_DNS
  Socket::dnsCache.waitShutdown(); // !SMT!-IP
#endif
  if(!exp) BufferedSocket::waitShutdown();

  QueueManager::getInstance()->saveQueue();
  SettingsManager::getInstance()->save();

  WebServerManager::deleteInstance();
  ClientProfileManager::deleteInstance();	
  HistoryManager::deleteInstance();
  IgnoreManager::deleteInstance();
  ADLSearchManager::deleteInstance();
  FinishedManager::deleteInstance();
  ShareManager::deleteInstance();
  CryptoManager::deleteInstance();
  DownloadManager::deleteInstance();
  UploadManager::deleteInstance();
  QueueManager::deleteInstance();
  ConnectionManager::deleteInstance();
  SearchManager::deleteInstance();
  FavoriteManager::deleteInstance();
  ClientManager::deleteInstance();
  HashManager::deleteInstance();
  QoSManager::deleteInstance();
  LogManager::deleteInstance();
  SettingsManager::deleteInstance();
  TimerManager::deleteInstance();
  DebugManager::deleteInstance();
  ResourceManager::deleteInstance();
  PGLoader::deleteInstance();
}

/**
 * @file
 * $Id: DCPlusPlus.cpp,v 1.5 2008/03/13 17:45:26 alexey Exp $
 */
