#include "stdinc.h"
#include "DCPlusPlus.h"
#include "PGLoader.h"
#include "Util.h"
#include "File.h"
#include "SettingsManager.h"
#include "LogManager.h"
#ifdef PPA_INCLUDE_IPFILTER
#include "peers/IpBlocksLoader.h"
#endif

#ifdef PPA_INCLUDE_PG
PGPlugin::~PGPlugin(){
	if(instance != NULL)
		FreeLibrary ( instance );
}

bool PGLoader::LoadPGDll(){
	HMODULE hr = LoadLibrary(Text::toT(Util::getDataPath() + "PeerGuardian.dll").c_str());
	if(!hr) {
		SettingsManager::getInstance()->set(SettingsManager::PG_ENABLE, false);
		return false;
	}

	typedef PGApi* (CALLBACK* LPFNDLLFUNC)();
	LPFNDLLFUNC pfn = (LPFNDLLFUNC)GetProcAddress(hr,(LPCSTR)"getObject");
	if(pfn == NULL) return false;

	pg= new PGPlugin();		

	if((pg->api = pfn()) == NULL){
		delete pg;
		return false;
	}
	pg->instance = hr;
	//MessageBox(0,_T("Guardian Loaded"), _T("Notice!"), 0);
	updateBlockList();
	return true;
}


PGLoader::PGLoader(){
	usePg = LoadPGDll();
}

bool PGLoader::notAbused() {
	if(usePg) {
		if((PGLoader::getIPBlockBool("1.1.1.1") && PGLoader::getIPBlockBool("255.255.255.255")) && (PGLoader::getIPBlock("1.1.1.1") == PGLoader::getIPBlock("255.255.255.255"))) {
			SettingsManager::getInstance()->set(SettingsManager::PG_ENABLE, false);
			return false;
		} else
			return true;
	} else {
		return true;
	}
}

string PGLoader::getIPBlock(const string aIP){ if(usePg){ return pg->api->ParseIP(aIP);} return ""; } 
bool PGLoader::getIPBlockBool(const string aIP){ 	if(usePg){ pg->api->ParseIP(aIP).empty() ? false : true; } return false;} 
bool PGLoader::isRunning(){ return usePg ? pg->api->isRunning() : false; }
bool PGLoader::isLoaded(){ return usePg; }
int PGLoader::getTotalIPRanges(){ return usePg ? pg->api->getTotalIPRanges() : 0; }
string PGLoader::getDescr(){ return usePg ? pg->api->getDescr() : ""; }
void PGLoader::updateBlockList(){ if(usePg) pg->api->ReloadBlockList(); }

#endif
//[+]PPA
#ifdef PPA_INCLUDE_IPFILTER
PGLoader::PGLoader(){
	LoadIPFilters();
}

bool PGLoader::getIPBlockBool(const string& p_IP) const {
  Lock l(m_cs); 
  if (m_IPTrust.size() == 0) {
    return false;	 
  }
  bool l_isBlock = true;
  unsigned u1, u2, u3, u4;
  const int iItems = sscanf_s(p_IP.c_str(), "%u.%u.%u.%u", &u1, &u2, &u3, &u4);
  if (iItems == 4) {
    const uint32_t l_ipnum = (u1 << 24) + (u2 << 16) + (u3 << 8) + u4;
    if (l_ipnum) {
      for (CustomIPFilterList::const_iterator j = m_IPTrust.begin(); j != m_IPTrust.end(); ++j) {
        if (l_ipnum >= j->m_startIP && l_ipnum <= j->m_endIP) {
#if 0
          if (j->m_is_block) {
            if (p_out_message.is_empty()) 
              p_out_message = ""
              p_out_message = j->m_message;
          }
#endif
          if (j->m_is_block) {
            return true;
          }
          else {
            l_isBlock &= false;
          }
        }
      }
    }
  }
  return l_isBlock;
}

void PGLoader::LoadIPFilters() {
  Lock l(m_cs); 
  m_IPTrust.clear();
#ifdef INCLUDE_PROVIDE_SELECTION
  const int provider = SETTING(PROVIDER);
  if (provider != 0 && provider != 1000) {
    __dcdebug("LoadIPFilters provider=%d\n", provider);
    IpBlocksLoader loader;
    parseAddressList(loader.getNameById(provider), loader.findById(provider));
  }
  else {
#endif
    loadIpTrustIni();
#ifdef INCLUDE_PROVIDE_SELECTION
  }
#endif
}

void PGLoader::loadIpTrustIni() {
  try {
    string file = Util::getConfigPath() + "IPTrust.ini";
    parseAddressList("IPTrust.ini", File(file, File::READ, File::OPEN).read());
  } 
  catch(const FileException&) {
  }
}

void PGLoader::parseAddressList(const string& context, const string& addressList) {
#ifdef _DEBUG
  LogManager::getInstance()->message("parseAddressList BEGIN");
#endif
  string data = addressList + " \n~~";
  string::size_type linestart = 0;
  string::size_type lineend = 0;
  for (;;) {
    lineend = data.find('\n', linestart);
    if (lineend == string::npos) break;
    const string line = data.substr(linestart, lineend - linestart - 1);
    if (!line.empty() && line[0] != '#' && line[0] != ';') {
      int u1, u2, u3, u4, u5, u6, u7, u8;
      const int iItems = sscanf_s(line.c_str(), "%u.%u.%u.%u - %u.%u.%u.%u", &u1, &u2, &u3, &u4, &u5, &u6, &u7, &u8);
      if (iItems == 8 || iItems == 4) {
        CustomIPFilter l_IPRange;
        if (u1 < 0) {
          l_IPRange.m_is_block = true;
          u1 = -u1;
        }
        else {
          l_IPRange.m_is_block = false;
        }
        l_IPRange.m_startIP = (u1 << 24) + (u2 << 16) + (u3 << 8) + u4;
#if 0
        const string::size_type l_start_comment = line.find(';',0);
        if(l_start_comment != string::npos)
          l_IPRange.m_message = line.substr(linestart, lineend - l_start_comment - 1);
#endif
        if (iItems == 4) {
          l_IPRange.m_endIP = l_IPRange.m_startIP;
        }
        else {
          l_IPRange.m_endIP = (u5 << 24) + (u6 << 16) + (u7 << 8) + u8;
        }
        m_IPTrust.push_back(l_IPRange);
        if (l_IPRange.m_is_block) {
          LogManager::getInstance()->message(context + " deny [" + line + "]");
        }
        else {
          LogManager::getInstance()->message(context + " allow [" + line + "]");
        }
      }
      else {
        LogManager::getInstance()->message(context + " error [" + line + "]");
      }
    }
#ifdef _DEBUG
    else {
      LogManager::getInstance()->message(context + " DEBUG skip [" + line + "]");
    }
#endif
    linestart = lineend + 1;
  }
#ifdef _DEBUG
  LogManager::getInstance()->message("parseAddressList END");
#endif
}
#endif
