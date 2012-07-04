////////////////////////////////////////////////
//	HistoryManager.h
//	
//	Added for persistent history management

#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "DCPlusPlus.h"

#include "Singleton.h"
#include "SettingsManager.h"
#include "FinishedManager.h"
#include "SimpleXML.h"

class HistoryManager: public Singleton<HistoryManager>, private SettingsManagerListener
{
public:
	HistoryManager() { SettingsManager::getInstance()->addListener(this); };
	~HistoryManager() { SettingsManager::getInstance()->removeListener(this); };

	// Search history
	void setSearchHistory(const TStringList& list);
	void clearSearchHistory();
	bool addSearchToHistory(const tstring& search);
	TStringList getSearchHistory() const;

private:
	// Search history
	void loadSearchHistory(SimpleXML& aXml);
	void saveSearchHistory(SimpleXML& aXml);
	TStringList	searchHistory;

	// Transfer histories
	void loadDownloadHistory();
	void loadUploadHistory();
	void saveDownloadHistory();
	void saveUploadHistory();

	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Load, SimpleXML& xml) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML& xml) throw();

	// Generic
	mutable CriticalSection cs;

};

#endif // HISTORYMANAGER_H
