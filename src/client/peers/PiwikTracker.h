#pragma once

#include "../Singleton.h"
#include <string>
#include <map>

class PiwikTracker: public ParametrizableSingleton<PiwikTracker, string>
{
	friend class ParametrizableSingleton<PiwikTracker, string>;
	PiwikTracker(const std::string& trackerURL);
	~PiwikTracker(void);

	string baseURL;
public:
	typedef map<string, string> actionVarsMap;

	void trackAction(const std::string& action, const actionVarsMap& cvars);
	void trackSearch(const std::string& searchKeyword, int searchResultsCount);
};
