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

#if !defined(DOWNLOAD_MANAGER_H)
#define DOWNLOAD_MANAGER_H


#include "TimerManager.h"

#include "UserConnection.h"
#include "Singleton.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "MerkleTree.h"
#include "QueueItem.h"

class ConnectionQueueItem;

/**
 * Comes as an argument in the DownloadManagerListener functions.
 * Use it to retrieve information about the ongoing transfer.
 */
class Download : public Transfer, public Flags {
public:
	static const string ANTI_FRAG_EXT;

	typedef Download* Ptr;
	typedef FlyLinkVector<Ptr> List;
	typedef List::const_iterator Iter;

	enum {
		FLAG_USER_LIST = 0x01,
		FLAG_RESUME = 0x02,
		FLAG_ROLLBACK = 0x04,
		FLAG_ZDOWNLOAD = 0x08,
		FLAG_CALC_CRC32 = 0x10,
		FLAG_CRC32_OK = 0x20,
		FLAG_ANTI_FRAG = 0x40,
		FLAG_TREE_DOWNLOAD = 0x100,
		FLAG_TREE_TRIED = 0x200,
		FLAG_PARTIAL_LIST = 0x400,
		FLAG_TESTSUR = 0x800,
		FLAG_CHECK_FILE_LIST = 0x1000,
		FLAG_MULTI_CHUNK = 0x2000,
		FLAG_PARTIAL = 0x4000,
		FLAG_TTH_CHECK = 0x8000,
		FLAG_CHUNKED = 0x10000
	};

	Download(UserConnection& conn) throw();
	Download(UserConnection& conn, QueueItem& qi, QueueItem::SourceConstIter aSource) throw();

	virtual void getParams(const UserConnection& aSource, StringMap& params);

	virtual ~Download();

	/** @return Target filename without path. */
	string getTargetFileName() const {
		return Util::getFileName(getTarget());
	}

	/** @internal */
	string getDownloadTarget() const {
		const string& tgt = (getTempTarget().empty() ? getTarget() : getTempTarget());
		return isSet(FLAG_ANTI_FRAG) ? tgt + ANTI_FRAG_EXT : tgt;			
	}

	int64_t getChunkSize() const { return getSize() - getStartPos(); }
	
	/** @internal */
	TigerTree& getTigerTree() { return tt; }
	string& getPFS() { return pfs; }
	/** @internal */
	AdcCommand getCommand(bool zlib);

	GETSET(string, source, Source);
	GETSET(string, target, Target);
	GETSET(string, tempTarget, TempTarget);
	GETSET(OutputStream*, file, File);
	GETSET(bool, treeValid, TreeValid);
	
private:
	Download(const Download&);
	Download& operator=(const Download&);

	TigerTree tt;
	string pfs;
};

/**
 * Use this listener interface to get progress information for downloads.
 *
 * @remarks All methods are sending a pointer to a Download but the receiver
 * (TransferView) is not using any of the methods in Download, only methods
 * from its super class, Transfer. The listener functions should send Transfer
 * objects instead.
 *
 * Changing this will will cause a problem with Download::List which is used
 * in the on Tick function. One solution is reimplement on Tick to call once
 * for every Downloads, sending one Download at a time. But maybe updating the
 * GUI is not DownloadManagers problem at all???
 */
class DownloadManagerListener {
public:
	virtual ~DownloadManagerListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> Status;

	/** 
	 * This is the first message sent before a download starts. 
	 * No other messages will be sent before.
	 */
	virtual void on(Starting, Download*) throw() { }

	/**
	 * Sent once a second if something has actually been downloaded.
	 */
	virtual void on(Tick, const Download::List&) throw() { }

	/** 
	 * This is the last message sent before a download is deleted. 
	 * No more messages will be sent after it.
	 */
	virtual void on(Complete, Download*, bool) throw() { }

	/** 
	 * This indicates some sort of failure with a particular download.
	 * No more messages will be sent after it.
	 *
	 * @remarks Should send an error code instead of a string and let the GUI
	 * display an error string.
	 */
	virtual void on(Failed, Download*, const string&) throw() { }
	virtual void on(Status, const UserPtr&, const string&) throw() { }
};


/**
 * Singleton. Use its listener interface to update the download list
 * in the user interface.
 */
class DownloadManager : public Speaker<DownloadManagerListener>, 
	private UserConnectionListener, private TimerManagerListener, 
	public Singleton<DownloadManager>
{
public:

	/** @internal */
	void addConnection(UserConnection::Ptr conn);
	void checkIdle(const UserPtr& user);

	/** @internal */
	void abortDownload(const string& aTarget, Download* except = NULL);

	/** @return Running average download speed in Bytes/s */
	int64_t getRunningAverage();

	/** @return Number of downloads. */ 
	size_t getDownloadCount() {
		Lock l(cs);
		return downloads.size();
	}

	bool startDownload(QueueItem::Priority prio);

        // todo: more generic
        bool checkFileDownload(const UserPtr& aUser); // !SMT!-S

	// the following functions were added to help download throttle
	inline bool throttle() const { return mThrottleEnable; }
	void throttleReturnBytes(uint32_t b);
	size_t throttleGetSlice();
	uint32_t throttleCycleTime();

private:
	void throttleZeroCounters();
	void throttleBytesTransferred(uint32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	uint32_t mCycleTime;
	int	   mBytesSpokenFor;
	size_t mBytesSent,
		   mDownloadLimit,
		   mByteSlice;
#ifdef PPA_INCLUDE_DROP_SLOW
        uint64_t m_lastSpeedCheckTick;
#endif
	
	enum { MOVER_LIMIT = 10*1024*1024 };
	class FileMover : public Thread {
	public:
		FileMover() : active(false) { }
		virtual ~FileMover() { join(); }

		void moveFile(const string& source, const string& target, bool streaming);
		virtual int run();
	private:
		class FilePair {
		private:
			string m_source;
			string m_target;
			bool m_streaming;
			friend class FileMover;
		public:
			FilePair() { }
			FilePair(string source, string target, bool streaming): m_source(source), m_target(target), m_streaming(streaming) { }
		};
		typedef vector<FilePair> FileList;
		typedef FileList::const_iterator FileIter;

		bool active;

		FileList files;
		CriticalSection cs;
	} mover;
	
	CriticalSection cs;
	Download::List downloads;
	UserConnection::List idlers;

	bool checkRollback(Download* aDownload, const uint8_t* aBuf, int aLen) throw(FileException);
	void removeConnection(UserConnection::Ptr aConn);
	void removeDownload(Download* aDown);
	void fileNotAvailable(UserConnection* aSource);
	void noSlots(UserConnection* aSource, string param = Util::emptyString);
	
	void moveFile(const string& source, const string&target, bool streaming);
	void logDownload(UserConnection* aSource, Download* d);
	int64_t getResumePos(const string& file, const TigerTree& tt, int64_t startPos);

	void failDownload(bool isError, UserConnection* aSource, const string& reason, bool connectSources = true);

	friend class Singleton<DownloadManager>;

	DownloadManager();
	~DownloadManager() throw();

	void checkDownloads(UserConnection* aConn, bool reconn = false);
	void handleEndData(UserConnection* aSource);

	// UserConnectionListener
	void on(Data, UserConnection*, const uint8_t*, size_t) throw();
	void on(Error, UserConnection*, const string&) throw();
	void on(Failed, UserConnection*, const string&) throw();
	void on(Sending, UserConnection*, int64_t) throw();
	void on(FileLength, UserConnection*, int64_t) throw();
	void on(MaxedOut, UserConnection*, string param = Util::emptyString) throw();
	void on(FileNotAvailable, UserConnection*) throw();
        void on(BanMessage, UserConnection*, const string& aMessage) throw(); // !SMT!-B
	void on(ListLength, UserConnection* aSource, const string& aListLength);
	
	void on(AdcCommand::SND, UserConnection*, const AdcCommand&) throw();
	void on(AdcCommand::STA, UserConnection*, const AdcCommand&) throw();

	bool prepareFile(UserConnection* aSource, int64_t newSize, bool z);
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint32_t aTick) throw();
};

#endif // !defined(DOWNLOAD_MANAGER_H)

/**
 * @file
 * $Id: DownloadManager.h,v 1.2 2008/04/01 02:39:15 alexey Exp $
 */
