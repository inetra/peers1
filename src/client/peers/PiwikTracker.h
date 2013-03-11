#pragma once

#include "../forward.h"
#include "../Singleton.h"
#include "../HttpConnection.h"
#include <string>
#include <map>
#include <vector>
#include "../TimerManager.h"

struct PiwikTrackerInfo {
	string url;
	int siteId;
	string siteName;

	PiwikTrackerInfo(const string& aUrl, const int aSiteId, const string& aSiteName) 
		: url(aUrl), siteId(aSiteId), siteName(aSiteName) {}

};

class PiwikTracker: public ParametrizableSingleton<PiwikTracker, PiwikTrackerInfo>,
	HttpConnectionListener, TimerManagerListener
{
	friend class ParametrizableSingleton<PiwikTracker, PiwikTrackerInfo>;
	PiwikTracker(const PiwikTrackerInfo& trackerInfo);
	virtual ~PiwikTracker(void);

	string baseUrl;
	string siteUrl;
	string userAgent;
	time_t startTime;
	vector<HttpConnection*> connections;
	vector<HttpConnection*> dead;
public:
	typedef map<string, string> varsMap;
	typedef map<string, string>::iterator varsIterator;
	typedef map<string, string>::const_iterator varsConstIterator;

	void trackAction(const string& action, const string &URI, const varsMap* vars = 0, const varsMap* cvars = 0);
	void shutdown();

	void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t*, size_t) throw() {}
	void on(HttpConnectionListener::Failed, HttpConnection* c, const string&) throw() { putConnection(c); }
	void on(HttpConnectionListener::Complete, HttpConnection* c, const string&) throw() { putConnection(c); }

	void on(TimerManagerListener::Second, uint32_t aTick) throw();
protected:
	string escapeJSON(const string& value);
	string mapToJSON(const varsMap& cvars);

	void putConnection(HttpConnection* c);

	CriticalSection cs;
	bool shuttingDown;
};
