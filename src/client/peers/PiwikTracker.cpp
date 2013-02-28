#include "stdinc.h"
#include "PiwikTracker.h"
#include "../SettingsManager.h"
#include "../CID.h"
#include "../Encoder.h"

PiwikTracker::PiwikTracker(const std::string& trackerURL) {
	string _id = string(Encoder::toHex(CID(SETTING(PRIVATE_ID)).data(), CID::SIZE), 0, 16); 
	baseURL = trackerURL + "&rec=1&_id=" + _id;
}

PiwikTracker::~PiwikTracker(void) {

}


void PiwikTracker::trackAction(const std::string& action, const actionVarsMap& cvars) {

}

void trackSearch(const std::string& searchKeyword, int searchResultsCount) {

}
