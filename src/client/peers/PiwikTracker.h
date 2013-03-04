#pragma once

#include "../forward.h"
#include "../Singleton.h"
#include "../HttpConnection.h"
#include <string>
#include <map>

struct PiwikTrackerInfo {
	string url;
	int siteId;
	string siteName;

	PiwikTrackerInfo(const string& aUrl, const int aSiteId, const string& aSiteName) 
		: url(aUrl), siteId(aSiteId), siteName(aSiteName) {}

};

class PiwikTracker: public ParametrizableSingleton<PiwikTracker, PiwikTrackerInfo>,
	HttpConnectionListener
{
	friend class ParametrizableSingleton<PiwikTracker, PiwikTrackerInfo>;
	PiwikTracker(const PiwikTrackerInfo& trackerInfo);
	~PiwikTracker(void);

	string baseUrl;
	string siteUrl;
	string userAgent;
	time_t startTime;
public:
	typedef map<string, string> varsMap;
	typedef map<string, string>::iterator varsIterator;
	typedef map<string, string>::const_iterator varsConstIterator;

	void trackAction(const string& action, const string &URI, const varsMap* vars = 0, const varsMap* cvars = 0);

	void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t*, size_t) throw() {}
	void on(HttpConnectionListener::Failed, HttpConnection* c, const string&) throw() { delete c; }
	void on(HttpConnectionListener::Complete, HttpConnection* c, const string&) throw() { delete c; }

protected:
	string escapeJSON(const string& value);
	string mapToJSON(const varsMap& cvars);

	CriticalSection cs;
};
