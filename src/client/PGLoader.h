#ifndef PGLOADER_H
#define PGLOADER_H

#include <string>
#include <vector>

#include "Singleton.h"
#include "CriticalSection.h"

#ifdef PPA_INCLUDE_PG

class PGApi{
public:
	PGApi(){}
	// Must supply a valid IPv4 (123.123.123.123)
	virtual bool isRunning(){return false;}

	virtual int getTotalIPRanges(){return 0;}
	virtual string getDescr(){return "";}
	virtual string ParseIP(const string& /*aIP*/){return "";}
	virtual void ReloadBlockList(){} // only necessary if block list has been changed..
};


class PGPlugin{
public:
	PGPlugin() : instance(NULL), api(NULL) {}
	virtual ~PGPlugin();
	HMODULE instance;
	PGApi* api;	
};

class PGLoader : public Singleton<PGLoader>{
public:
	PGLoader();
	~PGLoader(){}
	string getIPBlock(const string aIP); // Return company name if available(blocked)
	bool getIPBlockBool(const string aIP);
	bool isRunning();
	bool isLoaded();
	bool notAbused();
	int getTotalIPRanges();
	string getDescr();
	void updateBlockList();

private:
	bool usePg;
	bool LoadPGDll();
	friend class Singleton<PGLoader>;
	PGPlugin* pg;

};
#endif
#ifdef PPA_INCLUDE_IPFILTER
class PGLoader : public Singleton<PGLoader>{
public:
	enum {
		UNLIM_ANY_PROVIDER = 1000
	};
	PGLoader();
	~PGLoader()
	{
	}
	bool getIPBlockBool(const string& p_IP) const;
    void LoadIPFilters();
private:
  void loadIpTrustIni();
  void parseAddressList(const string& context, const string& addressList);
       mutable CriticalSection m_cs;
       struct CustomIPFilter
		{
                uint32_t m_startIP; 
				uint32_t m_endIP;
				bool     m_is_block;
#if 0
				string   m_message;
#endif
        };
        typedef vector<CustomIPFilter> CustomIPFilterList;
        CustomIPFilterList m_IPTrust;
};
#endif

#endif
