#include "stdinc.h"
#include "PiwikTracker.h"
#include "../SettingsManager.h"
#include "../CID.h"
#include "../Encoder.h"
#include "../QueueManager.h"
#include "../SearchManager.h"

#include "../peers/PeersUtils.h"
#include "../peers/PeersVersion.h"

#ifdef _WIN32
#include <windows.h>
#endif

class URL {
public:
	URL(string initURL) {
		url = initURL;
	};

	void addParam(const string& key, const string& value) {
		if (url.empty()) return;

		if (url.find('?') == string::npos) {
			url += "?" + key + "=";
		} else {
			url += "&" + key + "=";
		}

		string encodedValue;
		for (size_t i = 0; i < value.length(); ++i) {
			uint8_t c = value[i];
			if (isalnum(c)) {
				encodedValue += c;
			} else {
				encodedValue += "%" + Encoder::toHex(&c, 1);
			}
		}

		url += encodedValue;
	}

	string getEncodedUrl() { return url; }
private:
	string url;
};




PiwikTracker::PiwikTracker(const PiwikTrackerInfo& trackerInfo) {
	TimerManager::getInstance()->addListener(this);
	string _id = string(Encoder::toHex(CID(SETTING(PRIVATE_ID)).data(), CID::SIZE), 0, 16);
	URL url = URL(trackerInfo.url);
	url.addParam("idsite", Util::toString(trackerInfo.siteId));
	url.addParam("rec", "1");
	url.addParam("_id", _id);
	url.addParam("apiv", "1");
	siteUrl = "http://" + trackerInfo.siteName;

	userAgent = APPNAME "/" + Util::toString(BUILDID);
#ifdef _WIN32
	OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);
	userAgent += " (Windows NT " + Util::toString(osvi.dwMajorVersion) + "." + Util::toString(osvi.dwMinorVersion) + ")";

	int x = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	url.addParam("res", Util::toString(x) + "x" + Util::toString(y));
#endif

	baseUrl = url.getEncodedUrl();

	shuttingDown = false;
	startTime = GET_TIME();
	trackAction("start", "/");
}

PiwikTracker::~PiwikTracker(void) {

}


void PiwikTracker::trackAction(const string& action, const string &URI, const varsMap* vars, const varsMap* cvars) {
	Lock l(cs);
	if (shuttingDown) return;
	URL url = URL(baseUrl);
	url.addParam("url", siteUrl + URI);
	url.addParam("action_name", action);
	if (vars) {
		for (varsConstIterator i = vars->begin(); i != vars->end(); ++i) {
			url.addParam(i->first, i->second);
		}
	}
	if (cvars) {
		url.addParam("cvar", mapToJSON(*cvars));
	}
	HttpConnection *c = new HttpConnection(userAgent);
	if (c) {
		c->addListener(this);
		connections.push_back(c);
		c->downloadFile(url.getEncodedUrl());
	}
}

string PiwikTracker::escapeJSON(const string& value)
{
	const char *from = "\\" "\"";   // substitute for your string literal
	const char *to[] = { "\\\\", "\\\""};   // substitute for your string literals

	std::string t = value;
	
	for ( std::string::size_type pos = 0; pos < t.size(); /* */ )
	{
		pos = t.find_first_of( from, pos );
		if ( pos != std::string::npos )
		{
			std::string::size_type i = std::strchr( from, t[pos] ) - from;
			t.replace( pos, 1, to[i] );
			pos += std::strlen( to[i] );
		}
	}

	return ( t );
}

string PiwikTracker::mapToJSON(const varsMap& input) {
	string result = "{";
	int idx = 0;
	for (varsConstIterator i = input.begin(); i != input.end(); ++i, ++idx) {
		if (idx) result += ",";
		result += "\"" + Util::toString(idx+1) + "\":[\"" + escapeJSON(i->first) + "\",\"" + escapeJSON(i->second) + "\"]";
	}
	result += "}";
	return result;
}

void PiwikTracker::shutdown()
{
	TimerManager::getInstance()->removeListener(this);

	varsMap p;
	p["time"] = Util::toString(GET_TIME() - startTime);
	trackAction("exit", "/exit", 0, &p);

	{
		Lock l(cs);
		shuttingDown = true;

		for(vector<HttpConnection*>::iterator j = connections.begin(); j != connections.end(); ++j) {
			(*j)->cancelDownload();
		}
	}
	// Wait until all connections have died out...
	while(true) {
		{
			Lock l(cs);
			if(connections.empty()) {
				break;
			}
		}
		Thread::sleep(50);
	}

}

void PiwikTracker::putConnection(HttpConnection *c) {
	Lock l(cs);
	//c->removeListener(this);
	connections.erase(remove(connections.begin(), connections.end(), c), connections.end());
	dead.push_back(c);
}

void PiwikTracker::on(TimerManagerListener::Second, uint32_t aTick) throw() {	
	Lock l(cs);

	for(vector<HttpConnection*>::iterator j = connections.begin(); j != connections.end(); ++j) {
		if(((*j)->getLastActivity() + 10*1000) < aTick) 
		{
			(*j)->cancelDownload();
		}
	}

	for(vector<HttpConnection*>::iterator j = dead.begin(); j != dead.end(); ++j) {
		delete *j;
	}
	dead.clear();
}
