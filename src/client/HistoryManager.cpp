////////////////////////////////////////////////
//	HistoryManager.cpp
//	
//	Added for persistent history management

#include "stdinc.h"
#include "HistoryManager.h"

// Search history
void HistoryManager::loadSearchHistory(SimpleXML& aXml) {
	if(BOOLSETTING(AUTO_COMPLETE_SEARCH)) {
		if(aXml.findChild("SearchHistory")) {
			aXml.stepIn();
			while(aXml.findChild("Search")) {
				addSearchToHistory(Text::toT(aXml.getChildData()));
			}
			aXml.stepOut();
		}
	}
}

void HistoryManager::saveSearchHistory(SimpleXML& aXml) {
	if(BOOLSETTING(AUTO_COMPLETE_SEARCH)) {
		aXml.addTag("SearchHistory");
		aXml.stepIn();
		{
			Lock l(cs);
			for(TStringIter i = searchHistory.begin(); i != searchHistory.end(); ++i) {
				string tmp = Text::fromT(*i);
				aXml.addTag("Search", tmp);
			}
		}
		aXml.stepOut();
	}
}

void HistoryManager::setSearchHistory(const TStringList& list) {
	Lock l(cs);
	searchHistory = list;
}

void HistoryManager::clearSearchHistory() {
	Lock l(cs);
	searchHistory.clear();
}

bool HistoryManager::addSearchToHistory(const tstring& search) {
	if(search.empty())
		return false;

	Lock l(cs);

	if(find(searchHistory.begin(), searchHistory.end(), search) != searchHistory.end())
		return false;

		
	while(searchHistory.size() > static_cast<TStringList::size_type>(SETTING(SEARCH_HISTORY)))
		searchHistory.erase(searchHistory.begin());

	searchHistory.push_back(search);

	return true;
}

TStringList HistoryManager::getSearchHistory() const {
	Lock l(cs);
	return searchHistory;
}

// Transfer histories
void HistoryManager::loadDownloadHistory() {
	if(!BOOLSETTING(KEEP_DL_HISTORY))
		return;

	SimpleXML aXml;
	aXml.fromXML(File(Util::getConfigPath() + "FinishedDL.xml", File::READ, File::OPEN).read());
	if(aXml.findChild("FinishedDownloads")) {
		aXml.stepIn();
		while(aXml.findChild("Item")) {
			FinishedItem *i = new FinishedItem(aXml.getChildAttrib("Target"), aXml.getChildAttrib("User"), CID(aXml.getChildAttrib("CID")),
			aXml.getChildAttrib("Hub"), aXml.getIntChildAttrib("Size"), aXml.getIntChildAttrib("ChunkSize"), aXml.getIntChildAttrib("MilliSeconds"),
			aXml.getIntChildAttrib("Time"), aXml.getBoolChildAttrib("Crc32Checked"), aXml.getChildAttrib("TTH"));
			{
				Lock l(cs);
				FinishedManager::getInstance()->insertHistoryItem(i);
			}
		}
	}
}

void HistoryManager::loadUploadHistory() {
	if(!BOOLSETTING(KEEP_UL_HISTORY))
		return;

	SimpleXML aXml;
	aXml.fromXML(File(Util::getConfigPath() + "FinishedUL.xml", File::READ, File::OPEN).read());
	if(aXml.findChild("FinishedUploads")) {
		aXml.stepIn();
		while(aXml.findChild("Item")) {
			FinishedItem *i = new FinishedItem(aXml.getChildAttrib("Target"), aXml.getChildAttrib("User"), CID(aXml.getChildAttrib("CID")),
			aXml.getChildAttrib("Hub"), aXml.getIntChildAttrib("Size"), aXml.getIntChildAttrib("ChunkSize"), aXml.getIntChildAttrib("MilliSeconds"),
			aXml.getIntChildAttrib("Time"), aXml.getBoolChildAttrib("Crc32Checked"), aXml.getChildAttrib("TTH"));
			{
				Lock l(cs);
				FinishedManager::getInstance()->insertHistoryItem(i, true);
			}
		}
	}
}

void HistoryManager::saveDownloadHistory() {
	if(!BOOLSETTING(KEEP_DL_HISTORY)) {
		File::deleteFile(Util::getConfigPath() + "FinishedDL.xml");
		return;
	}

	SimpleXML aXml;
	aXml.addTag("FinishedDownloads");
	aXml.stepIn();

	const FinishedItem::List& fl = FinishedManager::getInstance()->lockList();
	for(FinishedItem::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		aXml.addTag("Item");
		aXml.addChildAttrib("Target", (*i)->getTarget());
		aXml.addChildAttrib("User", (*i)->getUser());
		aXml.addChildAttrib("CID", (*i)->getCID().toBase32());
		aXml.addChildAttrib("Hub", (*i)->getHub());
		aXml.addChildAttrib("Size", (*i)->getSize());
		aXml.addChildAttrib("ChunkSize", (*i)->getChunkSize());
		aXml.addChildAttrib("MilliSeconds", (*i)->getMilliSeconds());
		aXml.addChildAttrib("Time", (*i)->getTime());
		aXml.addChildAttrib("Crc32Checked", (*i)->getCrc32Checked());
		aXml.addChildAttrib("TTH", (*i)->getTTH());
	}
	FinishedManager::getInstance()->unlockList();

	aXml.stepOut();

	try {
		string fname = Util::getConfigPath() + "FinishedDL.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(aXml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("HistoryManager::saveDownloadHistory: %s\n", e.getError().c_str());
	}
}

void HistoryManager::saveUploadHistory() {
	if(!BOOLSETTING(KEEP_UL_HISTORY)) {
		File::deleteFile(Util::getConfigPath() + "FinishedUL.xml");
		return;
	}

	SimpleXML aXml;
	aXml.addTag("FinishedUploads");
	aXml.stepIn();

	const FinishedItem::List& fl = FinishedManager::getInstance()->lockList(true);
	for(FinishedItem::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		aXml.addTag("Item");
		aXml.addChildAttrib("Target", (*i)->getTarget());
		aXml.addChildAttrib("User", (*i)->getUser());
		aXml.addChildAttrib("CID", (*i)->getCID().toBase32());
		aXml.addChildAttrib("Hub", (*i)->getHub());
		aXml.addChildAttrib("Size", (*i)->getSize());
		aXml.addChildAttrib("ChunkSize", (*i)->getChunkSize());
		aXml.addChildAttrib("MilliSeconds", (*i)->getMilliSeconds());
		aXml.addChildAttrib("Time", (*i)->getTime());
		aXml.addChildAttrib("Crc32Checked", (*i)->getCrc32Checked());
		aXml.addChildAttrib("TTH", (*i)->getTTH());
	}
	FinishedManager::getInstance()->unlockList();

	aXml.stepOut();

	try {
		string fname = Util::getConfigPath() + "FinishedUL.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(aXml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("HistoryManager::saveUploadHistory: %s\n", e.getError().c_str());
	}
}

// SettingsManagerListener
void HistoryManager::on(SettingsManagerListener::Load, SimpleXML& aXml) {
	loadSearchHistory(aXml);
	loadDownloadHistory();
	loadUploadHistory();
}

void HistoryManager::on(SettingsManagerListener::Save, SimpleXML& aXml) {
	saveSearchHistory(aXml);
	saveDownloadHistory();
	saveUploadHistory();
}
