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

#if !defined(QUEUE_ITEM_H)
#define QUEUE_ITEM_H

class QueueManager;
class Download;

#include "User.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "FileChunksInfo.h"
#include "HashManager.h"
#include "SettingsManager.h"

class QueueItem : public Flags, public FastAlloc<QueueItem> {
public:
	typedef QueueItem* Ptr;
	typedef deque<Ptr> List;
	typedef List::const_iterator Iter;
	typedef HASH_MAP<string*, Ptr, noCaseStringHash, noCaseStringEq> StringMap;
	typedef StringMap::const_iterator StringIter;
	typedef HASH_MAP_X(UserPtr, Ptr, User::HashFunction, equal_to<UserPtr>, less<UserPtr>) UserMap;
	typedef UserMap::const_iterator UserIter;
	typedef HASH_MAP_X(UserPtr, List, User::HashFunction, equal_to<UserPtr>, less<UserPtr>) UserListMap;
	typedef UserListMap::const_iterator UserListIter;

	enum Status {
		/** The queue item is waiting to be downloaded and can be found in userQueue */
		STATUS_WAITING,
		/** This item is being downloaded and can be found in running */
		STATUS_RUNNING
	};

	enum Priority {
		DEFAULT = -1,
		PAUSED = 0,
		LOWEST,
		LOW,
		NORMAL,
		HIGH,
		HIGHEST,
		LAST
	};

	enum FileFlags {
		/** Normal download, no flags set */
		FLAG_NORMAL = 0x00, 
		/** This download should be resumed if possible */
		FLAG_RESUME = 0x01,
		/** This is a user file listing download */
		FLAG_USER_LIST = 0x02,
		/** The file list is downloaded to use for directory download (used with USER_LIST) */
		FLAG_DIRECTORY_DOWNLOAD = 0x04,
		/** The file is downloaded to be viewed in the gui */
		FLAG_CLIENT_VIEW = 0x08,
		/** Flag to indicate that file should be viewed as a text file */
		FLAG_TEXT = 0x20,
		/** This file exists on the hard disk and should be prioritised */
		FLAG_EXISTS = 0x40,
		/** Match the queue against this list */
		FLAG_MATCH_QUEUE = 0x80,
		/** The file list downloaded was actually an .xml.bz2 list */
		FLAG_XML_BZLIST = 0x200,
		/** Test user for slotlocker */
		FLAG_TESTSUR = 0x400,
		/** Test user's file list for fake share */
		FLAG_CHECK_FILE_LIST = 0x800,
		/** The file can be downloaded from multiple sources simultaneously */
		FLAG_MULTI_SOURCE = 0x1000,
		/** Autodrop slow source is enabled for this file */
		FLAG_AUTODROP = 0x2000,
		/** Video to be viewed online */
		FLAG_ONLINE_VIDEO = 0x4000,
		/** Video to be viewed online */
		FLAG_VIDEO_ACTIVATED = 0x8000
	};

	class Source : public Flags {
	public:
		enum {
			FLAG_NONE = 0x00,
			FLAG_FILE_NOT_AVAILABLE = 0x01,
			FLAG_ROLLBACK_INCONSISTENCY = 0x02,
			FLAG_PASSIVE = 0x04,
			FLAG_REMOVED = 0x08,
			FLAG_CRC_FAILED = 0x10,
			FLAG_CRC_WARN = 0x20,
			FLAG_NO_TTHF = 0x40,
			FLAG_BAD_TREE = 0x80,
			FLAG_SLOW = 0x100,
			FLAG_NO_TREE = 0x200,
			FLAG_NO_NEED_PARTS = 0x400,
			FLAG_PARTIAL = 0x800,
			FLAG_TTH_INCONSISTENCY = 0x1000,
			FLAG_MASK = FLAG_FILE_NOT_AVAILABLE | FLAG_ROLLBACK_INCONSISTENCY 
				| FLAG_PASSIVE | FLAG_REMOVED | FLAG_CRC_FAILED | FLAG_CRC_WARN | FLAG_BAD_TREE
				| FLAG_SLOW | FLAG_NO_TREE | FLAG_TTH_INCONSISTENCY
		};

		Source(const UserPtr& aUser) : user(aUser), disconnectedAt(0) { }
		Source(const Source& aSource) : Flags(aSource), user(aSource.user), partialInfo(aSource.partialInfo), disconnectedAt(aSource.disconnectedAt) { }

		bool operator==(const UserPtr& aUser) const { return user == aUser; }
		UserPtr& getUser() { return user; }

		/**
		 * Source parts info
		 * Meaningful only when FLAG_PARTIAL is set
		 * If this source is not bad source, empty parts info means full file
		 */
		GETSET(PartsInfo, partialInfo, PartialInfo);
		GETSET(UserPtr, user, User);
		map<unsigned,tick_t> requestedBlocks;
		tick_t disconnectedAt;
	};

	typedef vector<Source> SourceList;
	typedef SourceList::iterator SourceIter;
	typedef SourceList::const_iterator SourceConstIter;

	QueueItem(const string& aTarget, int64_t aSize, 
		Priority aPriority, Flags::MaskType aFlag, int64_t aDownloadedBytes, time_t aAdded, const TTHValue& tth) :
	Flags(aFlag), target(aTarget), currentDownload(NULL), averageSpeed(0),
        averageSegmentSpeed(0),
	size(aSize), downloadedBytes(aDownloadedBytes), status(STATUS_WAITING), priority(aPriority), added(aAdded),
	tthRoot(tth), autoPriority(false), chunkInfo(NULL)
	{ 
#ifdef PPA_INCLUDE_DROP_SLOW
		setFlag(FLAG_AUTODROP);
#endif
		if(isSet(FLAG_USER_LIST) || isSet(FLAG_TESTSUR) || isSet(FLAG_CHECK_FILE_LIST) || (size < 2097153)) {
			unsetFlag(FLAG_MULTI_SOURCE);
		}
	}

	QueueItem(const QueueItem& rhs) : 
	Flags(rhs), target(rhs.target), tempTarget(rhs.tempTarget),
		size(rhs.size), downloadedBytes(rhs.downloadedBytes), status(rhs.status), priority(rhs.priority), currents(rhs.currents),
		currentDownload(rhs.currentDownload), added(rhs.added), tthRoot(rhs.tthRoot),
                averageSegmentSpeed(rhs.averageSegmentSpeed),
                m_segmentSpeedHistory(rhs.m_segmentSpeedHistory),
		averageSpeed(rhs.averageSpeed), autoPriority(rhs.autoPriority), chunkInfo(rhs.chunkInfo)
	{
	}

	~QueueItem() { 
	}

	size_t countOnlineUsers() const {
		size_t n = 0;
		SourceConstIter i = sources.begin();
		for(; i != sources.end(); ++i) {
			if(i->getUser()->isOnline())
				n++;
		}
		return n;
	}
	bool hasOnlineUsers() const { 
		SourceConstIter i = sources.begin();
		for(; i != sources.end(); ++i) {
			if(i->getUser()->isOnline())
				return true;
		}
		return false;
	}

	SourceList& getSources() { return sources; }
	const SourceList& getSources() const { return sources; }
	SourceList& getBadSources() { return badSources; }
	const SourceList& getBadSources() const { return badSources; }

	void getOnlineUsers(UserList& l) const  {
		for(SourceConstIter i = sources.begin(); i != sources.end(); ++i)
			if(i->getUser()->isOnline())
				l.push_back(i->getUser());
	}

	string getTargetFileName() const { return Util::getFileName(getTarget()); }

	SourceIter getSource(const UserPtr& aUser) { return find(sources.begin(), sources.end(), aUser); }
	SourceIter getBadSource(const UserPtr& aUser) { return find(badSources.begin(), badSources.end(), aUser); }
	SourceConstIter getSource(const UserPtr& aUser) const { return find(sources.begin(), sources.end(), aUser); }
	SourceConstIter getBadSource(const UserPtr& aUser) const { return find(badSources.begin(), badSources.end(), aUser); }

	bool isSource(const UserPtr& aUser) const { return getSource(aUser) != sources.end(); }
	bool isBadSource(const UserPtr& aUser) const { return getBadSource(aUser) != badSources.end(); }
	bool isBadSourceExcept(const UserPtr& aUser, Flags::MaskType exceptions) const {
		SourceConstIter i = getBadSource(aUser);
		if(i != badSources.end())
			return i->isAnySet(exceptions^Source::FLAG_MASK);
		return false;
	}
	
	void addCurrent(const UserPtr& aUser) {
		dcassert(isSource(aUser));
		currents.push_back(aUser);
	}
	
	bool isCurrent(const UserPtr& aUser) const {
		dcassert(isSource(aUser));
		return find(currents.begin(), currents.end(), aUser) != currents.end();
	}

	// All setCurrent(NULL) should be replaced with this
	void removeCurrent(const UserPtr& aUser) {
		dcassert(isSource(aUser));
		dcassert(find(currents.begin(), currents.end(), aUser) != currents.end());

		currents.erase(find(currents.begin(), currents.end(), aUser));
	}

	int64_t getDownloadedBytes() const {
		if(chunkInfo)
			return chunkInfo->getDownloadedSize();
		return downloadedBytes;
	}

	bool isRangeDownloaded(int64_t begin, int64_t end) {
		return chunkInfo && chunkInfo->isRangeDownloaded(begin, end);
	}

	void setDownloadedBytes(int64_t pos) {
		downloadedBytes = pos;
	}
	
	string getListName() const {
		dcassert(isSet(QueueItem::FLAG_USER_LIST));
		if(isSet(QueueItem::FLAG_XML_BZLIST)) {
			return getTarget() + ".xml.bz2";
		} else {
			return getTarget() + ".xml";
		}
	}

	const string& getTempTarget();
	void setTempTarget(const string& aTempTarget) {
		tempTarget = aTempTarget;
	}
	GETSET(string, target, Target);
	GETSET(int64_t, size, Size);
	GETSET(Status, status, Status);
	GETSET(Priority, priority, Priority);
	GETSET(UserList, currents, Currents);
	GETSET(Download*, currentDownload, CurrentDownload);
	GETSET(time_t, added, Added);
	GETSET(TTHValue, tthRoot, TTH);
	GETSET(bool, autoPriority, AutoPriority);
	GETSET(uint8_t, maxSegments, MaxSegments);
	GET_PROTECTED_SET(size_t, averageSpeed, AverageSpeed);
#ifdef PPA_INCLUDE_DROP_SLOW
	GET_PROTECTED_SET(size_t, averageSegmentSpeed, AverageSegmentSpeed);
#endif                  
public:
	FileChunksInfo::Ptr getChunkInfo() const { return chunkInfo; }
#ifdef PPA_INCLUDE_DROP_SLOW
        void registerSegmentSpeed(int64_t totalSpeed, size_t downloadCount, size_t maxSegmentSpeedCount);
#endif                  
private:
	string tempTarget;
	int64_t downloadedBytes;
        FileChunksInfo::Ptr chunkInfo;
#ifdef PPA_INCLUDE_DROP_SLOW
        struct SegmentSpeedHistoryRecord {
          int64_t m_totalSpeed;
          size_t m_downloadCount;
          SegmentSpeedHistoryRecord(int64_t totalSpeed, size_t downloadCount): m_totalSpeed(totalSpeed), m_downloadCount(downloadCount) { }
        };
        deque<SegmentSpeedHistoryRecord> m_segmentSpeedHistory;
        pair<int64_t, size_t> getRegisteredAverageSpeed();
#endif
public:
	QueueItem::Priority calculateAutoPriority(){
		
		if(getAutoPriority()){
			QueueItem::Priority p;
			int percent = (int)(getDownloadedBytes() * 10.0 / getSize());
			switch(percent){
					case 0:
					case 1:
/*						p = QueueItem::LOWEST;
						break;*/
					case 2:
					case 3:
						p = QueueItem::LOW;
						break;
					case 4:
					case 5:						
					case 6:
					default:
						p = QueueItem::NORMAL;
						break;
					case 7:
					case 8:
						p = QueueItem::HIGH;
						break;
					case 9:
					case 10:
						p = QueueItem::HIGHEST;			
						break;
			}
			return p;			
		}
		return priority;
	}

	bool hasFreeSegments() const {
		if(!isSet(QueueItem::FLAG_MULTI_SOURCE)) {
			return (status != STATUS_RUNNING);
		} else {
			return ((uint8_t(currents.size()) < maxSegments) 
				  //[!]PPA  &&
				  //[!]PPA	(!BOOLSETTING(DONT_BEGIN_SEGMENT) || ((size_t)(SETTING(DONT_BEGIN_SEGMENT_SPEED)*1024) >= averageSpeed))
					);
		}
	}

private:
	QueueItem& operator=(const QueueItem&);

	friend class QueueManager;
        friend class DownloadManager;
	SourceList sources;
	SourceList badSources;

	void addSource(const UserPtr& aUser);
	void removeSource(const UserPtr& aUser, int reason);
};

#endif // !defined(QUEUE_ITEM_H)

/**
* @file
* $Id: QueueItem.h,v 1.5 2008/04/01 02:41:14 alexey Exp $
*/
