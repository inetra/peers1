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

#if !defined(HASH_MANAGER_H)
#define HASH_MANAGER_H

#include "Singleton.h"
#include "MerkleTree.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "Semaphore.h"
#include "TimerManager.h"
#include "Util.h"
#include "FastAlloc.h"
#include "Text.h"
#include "Streams.h"

STANDARD_EXCEPTION(HashException);
class File;

class HashManagerListener {
public:
	virtual ~HashManagerListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> TTHDone;

	virtual void on(TTHDone, const string& /* fileName */, const TTHValue& /* root */) throw() = 0;
};

class HashLoader;
class FileException;

class HashManager : public Singleton<HashManager>, public Speaker<HashManagerListener>,
	private TimerManagerListener 
{
public:

	/** We don't keep leaves for blocks smaller than this... */
	static const int64_t MIN_BLOCK_SIZE;

	HashManager() {
		TimerManager::getInstance()->addListener(this);
	}
	virtual ~HashManager() throw() {
		TimerManager::getInstance()->removeListener(this);
		hasher.join();
	}

	/**
	 * Check if the TTH tree associated with the filename is current.
	 */
	bool checkTTH(const string& fname,const string& fpath, int64_t aSize, uint32_t aTimeStamp);

	void stopHashing(const string& baseDir) { hasher.stopHashing(baseDir); }
	void setPriority(Thread::Priority p) { hasher.setThreadPriority(p); }

	/** @return TTH root */
	const TTHValue& getTTH(const string& fname,const string& fpath, int64_t aSize) throw(HashException);

	bool getTree(const TTHValue& root, TigerTree& tt);

	void addTree(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tt) {
		hashDone(aFileName, aTimeStamp, tt, -1);
	}
	void addTree(const TigerTree& tree) { Lock l(cs); store.addTree(tree); }

	void getStats(string& curFile, int64_t& bytesLeft, size_t& filesLeft) {
		hasher.getStats(curFile, bytesLeft, filesLeft);
	}

	/**
	 * Rebuild hash data file
	 */
	void rebuild() { hasher.scheduleRebuild(); }

	void startup() { hasher.start(); store.load(); }

	void shutdown() {
		hasher.shutdown();
		hasher.join();
		Lock l(cs);
		store.save();
	}

private:

	class Hasher : public Thread {
	private:

#ifdef _WIN32
		enum { BUF_SIZE = 4*1024*1024 };
#endif

		class HasherShutdown : public Exception {
		};

		class FileHashBufferFactory {
		private:
			uint8_t* m_virtualBuffer;
		public:
			FileHashBufferFactory(): m_virtualBuffer(NULL) {
			}

			uint8_t* allocate() {
				if (m_virtualBuffer == NULL) {
					m_virtualBuffer = (uint8_t*) VirtualAlloc(NULL, 2 * BUF_SIZE, MEM_COMMIT, PAGE_READWRITE);
				}
				return m_virtualBuffer;
			}

			uint8_t* get() {
				return m_virtualBuffer;
			}

			~FileHashBufferFactory() {
				if (m_virtualBuffer != NULL) {
					VirtualFree(m_virtualBuffer, 0, MEM_RELEASE);
				}
			}

		};

		class FileHashBuffer {
			enum MemoryMode { mmVirtual, mmShared, mmOwn };
		private:
			uint8_t* m_ptr;
			size_t m_size;
			MemoryMode m_memoryMode;
		public:
			FileHashBuffer(FileHashBufferFactory& factory, int64_t fileSize, bool allowVirtualBuffer) {
				if (allowVirtualBuffer && fileSize > 2 * BUF_SIZE) {
					m_ptr = factory.allocate();
					if (m_ptr != NULL) {
						m_size = 2 * BUF_SIZE;
						m_memoryMode = mmVirtual;
						return;
					}
				}
				else {
					m_ptr = factory.get();
					if (m_ptr != NULL) {
						m_size = 2 * BUF_SIZE;
						m_memoryMode = mmShared;
						return;
					}
				}
				m_memoryMode = mmOwn;
				const size_t bufferSize = fileSize < BUF_SIZE ? (size_t) fileSize : BUF_SIZE;
				m_ptr = new uint8_t[bufferSize];
				m_size = bufferSize;
			}

			FileHashBuffer(FileHashBuffer& src): m_ptr(src.m_ptr), m_size(src.m_size), m_memoryMode(src.m_memoryMode) {
				src.m_ptr = NULL;
			}

			void operator = (FileHashBuffer& src) {
				m_ptr = src.m_ptr;
				m_size = src.m_size;
				m_memoryMode = src.m_memoryMode;
				src.m_ptr = NULL;
			}

			~FileHashBuffer() {
				if (m_ptr != NULL && m_memoryMode == mmOwn) {
					delete[] m_ptr;
				}
			}

			operator uint8_t* () const {
				return m_ptr;
			}

			size_t size() const {
				return m_size;
			}

			bool isVirtual() const {
				return m_memoryMode == mmVirtual;
			}
		};

		FileHashBufferFactory bufferFactory;

	public:
		Hasher() : stop(false), running(false), rebuild(false), currentSize(0), startTime(0), workTime(0) { }

		void hashFile(const string& fileName, int64_t size);

		void stopHashing(const string& baseDir);
		virtual int run();
		void getStats(string& curFile, int64_t& bytesLeft, size_t& filesLeft);
		void shutdown() { stop = true; s.signal(); }
		void scheduleRebuild() { rebuild = true; s.signal(); }

	private:
		// Case-sensitive (faster), it is rather unlikely that case changes, and if it does it's harmless.
		// map because it's sorted (to avoid random hash order that would create quite strange shares while hashing)
		typedef map<string, int64_t> WorkMap;	
		typedef WorkMap::iterator WorkIter;

		WorkMap w;
		FastCriticalSection cs;
		Semaphore s;

		volatile bool stop;
		volatile bool running;
		bool rebuild;
		string currentFile;
		int64_t currentSize;
		tick_t workTime;
		tick_t startTime;

		void processFile(const string& fname);

		bool fastHash(const string& fname, uint8_t* buf, TigerTree& tth, int64_t size, uint64_t& timestamp);

		inline void checkSpeed() {
			if (Util::isMinimized()) {
				limitSpeed();
			}
		}

		void limitSpeed();
		
#ifdef _WIN32
		inline BOOL GetOverlappedResult(__in  HANDLE hFile, __in  LPOVERLAPPED lpOverlapped, __out LPDWORD lpNumberOfBytesTransferred) {
			const tick_t startWaitTime = GET_TICK();
			BOOL result = ::GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, TRUE);
			workTime += (GET_TICK() - startWaitTime);
			return result;
		}
#endif

		string nextQueueItem() {
			FastLock l(cs);
			if (!w.empty()) {
				WorkMap::iterator head = w.begin();
				currentFile = head->first;
				currentSize = head->second;
				w.erase(head);
				running = true;
				return currentFile;
			}
			else {
				return Util::emptyString;
			}
		}

		void clearRunningState() {
			FastLock l(cs);
			currentFile.clear();
			currentSize = 0;
			running = false;
		}
	};

	friend class Hasher;

	class HashStore {
	public:
		HashStore();
		void addFile(const string& aFileName, uint64_t aTimeStamp, const TigerTree& tth, bool aUsed);

		void load();
		void save();

		void rebuild();

		bool checkTTH(const string& aFileName, const string& aFilePath, int64_t aSize, uint32_t aTimeStamp);

		void addTree(const TigerTree& tt) throw();
		const TTHValue* getTTH(const string& fname,const string& fpath);
		bool getTree(const TTHValue& root, TigerTree& tth);
		bool isDirty() { return dirty; }
	private:
		/** Root -> tree mapping info, we assume there's only one tree for each root (a collision would mean we've broken tiger...) */
		struct TreeInfo {
			TreeInfo() : size(0), index(0), blockSize(0) { }
			TreeInfo(int64_t aSize, int64_t aIndex, int64_t aBlockSize) : size(aSize), index(aIndex), blockSize(aBlockSize) { }
			TreeInfo(const TreeInfo& rhs) : size(rhs.size), index(rhs.index), blockSize(rhs.blockSize) { }
			TreeInfo& operator=(const TreeInfo& rhs) { size = rhs.size; index = rhs.index; blockSize = rhs.blockSize; return *this; }

			GETSET(int64_t, size, Size);
			GETSET(int64_t, index, Index);
			GETSET(int64_t, blockSize, BlockSize);
		};

		/** File -> root mapping info */
		struct FileInfo {
		public:
			FileInfo(const string& aFileName, const TTHValue& aRoot, uint64_t aTimeStamp, bool aUsed) :
			  fileName(aFileName), root(aRoot), timeStamp(aTimeStamp), used(aUsed) { }

			bool operator==(const string& name) { return name == fileName; }

			GETSET(string, fileName, FileName);
			GETSET(TTHValue, root, Root);
			GETSET(uint64_t, timeStamp, TimeStamp);
			GETSET(bool, used, Used);
		};

		typedef vector<FileInfo> FileInfoList;
		typedef FileInfoList::iterator FileInfoIter;

		typedef HASH_MAP<string, FileInfoList> DirMap;
		typedef DirMap::iterator DirIter;

		typedef HASH_MAP_X(TTHValue, TreeInfo, TTHValue::Hash, equal_to<TTHValue>, less<TTHValue>) TreeMap;
		typedef TreeMap::iterator TreeIter;

		friend class HashLoader;

		DirMap fileIndex;
		TreeMap treeIndex;

		bool dirty;

		void createDataFile(const string& name);

		bool loadTree(File& dataFile, const TreeInfo& ti, const TTHValue& root, TigerTree& tt);
		int64_t saveTree(File& dataFile, const TigerTree& tt) throw(FileException);

		string getIndexFile() { return Util::getConfigPath() + "HashIndex.xml"; }
		string getDataFile() { return Util::getConfigPath() + "HashData.dat"; }
	};

	friend class HashLoader;

	Hasher hasher;
	HashStore store;

	CriticalSection cs;

	/** Single node tree where node = root, no storage in HashData.dat */
	static const int64_t SMALL_TREE = -1;
	
	static string prepareFileName(const string& aFileName);
	void hashDone(const string& aFileName, uint64_t aTimeStamp, const TigerTree& tth, int64_t speed);
	void doRebuild() {
		Lock l(cs);
		store.rebuild();
	}
	virtual void on(TimerManagerListener::Minute, uint32_t) throw() {
		Lock l(cs);
		store.save();
	}
};

#endif // !defined(HASH_MANAGER_H)

/**
 * @file
 * $Id: HashManager.h,v 1.8 2008/04/19 05:33:14 alexey Exp $
 */
