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

#if !defined(QUEUE_MANAGER_H)
#define QUEUE_MANAGER_H

#include "TimerManager.h"

#include "CriticalSection.h"
#include "Exception.h"
#include "User.h"
#include "File.h"
#include "QueueItem.h"
#include "Singleton.h"
//#include "DirectoryListing.h"
#include "MerkleTree.h"

#include "QueueManagerListener.h"
#include "SearchManagerListener.h"
#include "ClientManagerListener.h"
#include "LogManager.h"

STANDARD_EXCEPTION(QueueException);

class UserConnection;
class DirectoryListing;

class DirectoryItem {
public:
	typedef DirectoryItem* Ptr;
	typedef HASH_MULTIMAP<UserPtr, Ptr, User::HashFunction> DirectoryMap;
	typedef DirectoryMap::const_iterator DirectoryIter;
	typedef pair<DirectoryIter, DirectoryIter> DirectoryPair;
	
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	DirectoryItem() : priority(QueueItem::DEFAULT) { }
	DirectoryItem(const UserPtr& aUser, const string& aName, const string& aTarget, 
		QueueItem::Priority p) : name(aName), target(aTarget), priority(p), user(aUser) { }
	~DirectoryItem() { }
	
	UserPtr& getUser() { return user; }
	void setUser(const UserPtr& aUser) { user = aUser; }
	
	GETSET(string, name, Name);
	GETSET(string, target, Target);
	GETSET(QueueItem::Priority, priority, Priority);
private:
	UserPtr user;
};

class ConnectionQueueItem;
class QueueLoader;

class QueueManager : public ParametrizableSingleton<QueueManager,string>, public Speaker<QueueManagerListener>, private TimerManagerListener, 
	private SearchManagerListener, private ClientManagerListener
{
public:
	/** Add a file with hash to the queue. */
	bool add(const string& aFile, int64_t aSize, const string& tth, const int additionalFlags = 0) throw(QueueException, FileException); 
	/** Add a file to the queue. */
	void add(const string& aTarget, int64_t aSize, const TTHValue& root, UserPtr aUser,
		int aFlags = QueueItem::FLAG_RESUME, bool addBad = true) throw(QueueException, FileException);
		/** Add a user's filelist to the queue. */
	void addList(const UserPtr& aUser, int aFlags) throw(QueueException, FileException);
	/** Queue a partial file list download */
	void addPfs(const UserPtr& aUser, const string& aDir) throw(QueueException);

	void addTestSUR(UserPtr aUser, bool checkList = false) throw(QueueException, FileException) {
		string fileName = "TestSUR" + Util::validateFileName(Util::cleanPathChars(aUser->getFirstNick()))  + "." + aUser->getCID().toBase32();
		string target = Util::getConfigPath() + "TestSURs\\" + fileName;
		add(target, -1, TTHValue(), aUser, (checkList ? QueueItem::FLAG_CHECK_FILE_LIST : 0) | QueueItem::FLAG_TESTSUR);
	}

	void removeTestSUR(UserPtr aUser) {
		try {
			string fileName = "TestSUR" + Util::validateFileName(Util::cleanPathChars(aUser->getFirstNick()))  + "." + aUser->getCID().toBase32();
			string target = Util::getConfigPath() + "TestSURs\\" + fileName;
			remove(target);
		} catch(...) {
			// exception
		}
		return;
	}

	/** Readd a source that was removed */
	void readd(const string& target, UserPtr& aUser) throw(QueueException);
	/** Add a directory to the queue (downloads filelist and matches the directory). */
	void addDirectory(const string& aDir, const UserPtr& aUser, const string& aTarget, QueueItem::Priority p = QueueItem::DEFAULT) throw();
	
	int matchListing(const DirectoryListing& dl) throw();

	bool getTTH(const string& name, TTHValue& tth) throw();

	/** Move the target location of a queued item. Running items are silently ignored */
	void move(const string& aSource, const string& aTarget) throw();

	void remove(const string& aTarget) throw();
	void removeSource(const string& aTarget, UserPtr& aUser, int reason, bool removeConn = true) throw();
	void removeSource(const UserPtr& aUser, int reason) throw();

	void setPriority(const string& aTarget, QueueItem::Priority p) throw();
	void setAutoPriority(const string& aTarget, bool ap) throw();

   typedef set<TTHValue,less<TTHValue> > TTHSet; // !SMT!-UI
   void getQueue(TTHSet &dst); // !SMT!-UI

	void getTargets(const TTHValue& tth, StringList& ql);
	void getTargets(const TTHValue& tth, QueueItem::List& sl);
	QueueItem::StringMap& lockQueue() throw() { cs.enter(); return fileQueue.getQueue(); } ;
	void unlockQueue() throw() { cs.leave(); }

	bool getQueueInfo(UserPtr& aUser, string& aTarget, int64_t& aSize, int& aFlags, bool& aFileList, bool& aSegmented) throw();
	Download* getDownload(UserConnection& aSource, string& message) throw();
	void putDownload(Download* aDownload, bool finished, bool connectSources = true) throw();
	void downloadFinished(Download* aDownload) throw();

	/** @return The highest priority download the user has, PAUSED may also mean no downloads */
	QueueItem::Priority hasDownload(const UserPtr& aUser) throw();
	
	void loadQueue() throw();
	void saveQueue() throw();

	void doSearch(uint32_t aTick) throw();

	bool handlePartialSearch(const TTHValue& tth, FileChunksInfo::Ptr& _outChunksInfo);
	bool handlePartialResult(const UserPtr& aUser, const TTHValue& tth, const PartialPeer& partialPeer, PartsInfo& partialInfo, PartsInfo& runInfo);
	
#ifdef PPA_INCLUDE_DROP_SLOW
	void dropSourceWithoutLock(QueueItem* q, Download* d);
	QueueItem* getQueueItemWithoutLock(Download* download);
#endif
	bool isStreamingDownload(const string& target);
	QueueItem::List getRunningFiles() throw() {
		QueueItem::List ql;
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* q = i->second;
			if(q->getStatus() == QueueItem::STATUS_RUNNING) {
				ql.push_back(q);
			}
		}
		return ql;
	}

	bool hasTempTarget(const string& tempTarget) throw() {
		Lock l(cs);
		for(QueueItem::StringIter i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i) {
			QueueItem* q = i->second;
			if (q->getTempTarget() == tempTarget) {
				return true;
			}
		}
		return false;
	}

	bool getTargetByRoot(const TTHValue& tth, string& target, string& tempTarget) {
		Lock l(cs);
		QueueItem::List ql;
		fileQueue.find(ql, tth);

		if(ql.empty()) return false;

		target = ql.front()->getTarget();
		tempTarget = ql.front()->getTempTarget();
		return true;
	}
	
	GETSET(uint64_t, lastSave, LastSave);
	GETSET(string, queueFile, QueueFile);

	typedef HASH_MAP_X(CID, string, CID::Hash, equal_to<CID>, less<CID>) PfsQueue;
	typedef PfsQueue::iterator PfsIter;

	/** All queue items by target */
	class FileQueue {
	public:
		FileQueue() /*: lastInsert(queue.end())*/ { }
		~FileQueue() {
			for(QueueItem::StringIter i = queue.begin(); i != queue.end(); ++i)
				delete i->second;
			}
		void add(QueueItem* qi);
		QueueItem* add(const string& aTarget, int64_t aSize, 
			Flags::MaskType aFlags, QueueItem::Priority p, const string& aTempTarget, int64_t aDownloaded,
			time_t aAdded, const string& freeBlocks, const string& verifiedBlocks, const TTHValue& root) throw(QueueException, FileException);

		QueueItem* find(const string& target);
		void find(QueueItem::List& sl, int64_t aSize, const string& ext);
		uint8_t getMaxSegments(int64_t filesize);
		void find(StringList& sl, int64_t aSize, const string& ext);
		void find(QueueItem::List& ql, const TTHValue& tth);

		QueueItem* findAutoSearch(deque<string>& recent);
		size_t getSize() { return queue.size(); }
		QueueItem::StringMap& getQueue() { return queue; }
		void move(QueueItem* qi, const string& aTarget);
		void remove(QueueItem* qi) {
			queue.erase(const_cast<string*>(&qi->getTarget()));

			if(qi->isSet(QueueItem::FLAG_MULTI_SOURCE)) {
				qi->chunkInfo = NULL;
				FileChunksInfo::Free(&qi->getTTH());
			}

			delete qi;
		}

	private:
		QueueItem::StringMap queue;
	};

	/** QueueItems by target */
	FileQueue fileQueue;

private:
	/** All queue items indexed by user (this is a cache for the FileQueue really...) */
	class UserQueue {
	public:
		void add(QueueItem* qi);
		void add(QueueItem* qi, const UserPtr& aUser);
		QueueItem* getNext(const UserPtr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST, QueueItem* pNext = NULL);
		QueueItem* getNextAll(const UserPtr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST);
		QueueItem* getRunning(const UserPtr& aUser);
		void setRunning(QueueItem* qi, const UserPtr& aUser);
		void setWaiting(QueueItem* qi, const UserPtr& aUser);
		QueueItem::UserListMap& getList(int p) { return userQueue[p]; }
		void remove(QueueItem* qi);
		void remove(QueueItem* qi, const UserPtr& aUser);

		QueueItem::UserMap& getRunning() { return running; }
		bool isRunning(const UserPtr& aUser) const { 
			return (running.find(aUser) != running.end());
		}
	private:
		/** QueueItems by priority and user (this is where the download order is determined) */
		QueueItem::UserListMap userQueue[QueueItem::LAST];
		/** Currently running downloads, a QueueItem is always either here or in the userQueue */
		QueueItem::UserMap running;
	};

	friend class QueueLoader;
	friend class ParametrizableSingleton<QueueManager,string>;

        string m_version;
	
	QueueManager(const string& version);
	virtual ~QueueManager() throw();
	
	mutable CriticalSection cs;
	
	/** Partial file list queue */
	PfsQueue pfsQueue;
	/** QueueItems by user */
	UserQueue userQueue;
	/** Directories queued for downloading */
	DirectoryItem::DirectoryMap directories;
	/** Recent searches list, to avoid searching for the same thing too often */
	deque<string> recent;
	/** The queue needs to be saved */
	bool dirty;
	/** Next search */
	uint32_t nextSearch;
	
	/** Sanity check for the target filename */
	static string checkTarget(const string& aTarget, int64_t aSize, int& flags) throw(QueueException, FileException);
	/** Add a source to an existing queue item */
	bool addSource(QueueItem* qi, UserPtr aUser, Flags::MaskType addBad) throw(QueueException, FileException);

	void processList(const string& name, UserPtr& user, int flags);

	void setDirty() {
		if(!dirty) {
			dirty = true;
			lastSave = GET_TICK();
		}
	}

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, uint32_t aTick) throw();
	virtual void on(TimerManagerListener::Minute, uint32_t aTick) throw();
	
	// SearchManagerListener
	virtual void on(SearchManagerListener::SR, SearchResult*) throw();

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw();
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw();
};

#endif // !defined(QUEUE_MANAGER_H)

/**
 * @file
 * $Id: QueueManager.h,v 1.4 2008/04/01 02:39:15 alexey Exp $
 */
