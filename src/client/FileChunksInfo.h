/* 
 * Copyright (C) 2003-2005 RevConnect, http://www.revconnect.com
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

/*
 * Terms:
 *     Chunk  : file bytes range
 *     Block  : TTH level-10 blocks range
 *     Running: a chunk status of asking slot or downloading, occupied
 *     Waiting: a chunk not running
 *     Pending: running chunk without any progress
 *     Split  : when no waiting chunk available, a running chunk is split 
 *              into two chunks
 *     PFS    : partial file sharing
 */

#ifndef FILECHUNKSINFO_H_
#define FILECHUNKSINFO_H_

#include "Pointer.h"
#include "CriticalSection.h"
#include "MerkleTree.h"

// minimum file size to be PFS : 20M
#define PARTIAL_SHARE_MIN_SIZE 20971520

// it's used when a source's download speed is unknown
#define DEFAULT_SPEED 5120

// maximum number of partial peers to store, to minimise unnecessary UDP
#define MAX_PARTIAL_PEERS 20

// PFS purpose
typedef vector<uint16_t> PartsInfo;

// ...
typedef map<uint16_t, uint16_t> BlockMap;
typedef BlockMap::const_iterator BlockIter;

// Aversion purpose
typedef vector<uint16_t> BlockVector;

class Download;


/**
 * Hold basic information of a chunk
 * Features: 
 *     . download speed awareness when chunk split
 *     . overlapped
 */
class Chunk
{
private:
	friend class FileChunksInfo;

	typedef map<int64_t, Chunk*> Map;
	typedef Map::const_iterator  Iter;

	Chunk(int64_t startOffset, int64_t endOffset) 
		: download(NULL), pos(startOffset), end(endOffset), overlappedCount(0)
	{}

	int64_t pos;
	int64_t end;

	Download* download;
	
	// allow overlapped download the same pending chunk 
	// when all running chunks could not be split
	uint8_t overlappedCount;
};

class PartialPeer {
public:
	PartialPeer() { }
	PartialPeer(const string& pMyNick, const string& pHubIpPort, const string& pTth, const string& pAddress, uint16_t pUdpPort, bool pNmdc) : 
	  myNick(pMyNick), hubIpPort(pHubIpPort), tth(pTth), address(pAddress), udpPort(pUdpPort), nmdc(pNmdc), nextQueryTime(0) { }

	typedef vector<PartialPeer*> List;
	typedef List::iterator		 Iter;

    //copy constuctor (see if it works implicitly
	//PartialPeer(const PartialPeer& pp) : myNick(pp.myNick), hubIpPort(pp.hubIpPort), tth(pp.tth), address(pp.Address), udpPort(pUdpPort), nmdc(pNmdc), nextQueryTime(0) { }

	GETSET(string, myNick, MyNick);
	GETSET(string, hubIpPort, HubIpPort);
	GETSET(string, tth, Tth);
	GETSET(string, address, Address);
	GETSET(uint16_t, udpPort, UdpPort);
	GETSET(bool, nmdc, Nmdc);
	GETSET(uint64_t, nextQueryTime, NextQueryTime);

	string getDump()
	{
		return myNick + hubIpPort + tth + address;
	}

	bool equals(const PartialPeer& other)
	{	
		return (address == other.address) && (udpPort == other.udpPort);
	}
};

/**
 * Holds multi-sources downloading file information
 * including downloaded chunks and verified parts/blocks
 */

class FileChunksInfo : public PointerBase
{
public:
	typedef Pointer<FileChunksInfo> Ptr;

	enum{
		NORMAL_EXIT,
		CHUNK_OVER,
		FILE_OVER,
        CHUNK_LOST
	} CHUNK_RESULT;

	/**
     * Get file chunks information by file name
     */
	static Ptr Get(const TTHValue* tth);

	/**
     * Free the file chunks information, this is called when target is removed from download queue
     * See also ~FileChunksInfo()
     */
    static void Free(const TTHValue* tth);

	/**
     * Create file chunks infromation with name, size and chunks detail
     */
	FileChunksInfo(const TTHValue* tth, int64_t size, const vector<int64_t>* blocks, bool streaming);

	/**
     * Smart pointer based destructor
     */
	~FileChunksInfo();

	/**
     * Get start offset of a free chunk, the end offset of chunk is unpredictable
     */
	int64_t getChunk(bool& useChunks, int64_t _speed, bool streaming, map<unsigned,tick_t> &requestedBlocks);

	/**
	 * Compute download start position by source's parts info
	 *
	 * Return the 1st duplicate position both in free and parts info
	 * Or split the biggest duplicate chunk both in running and parts info
	 */
	int64_t getChunk(const PartsInfo& partialInfo, int64_t _speed, bool streaming, map<unsigned,tick_t> &requestedBlocks);

	/**
     * Abandon all unverified bytes received by a chunk
     */
	void abandonChunk(int64_t);
	
	/**
     * Release the chunk with start offset
     */
	void putChunk(int64_t);

	/** 
	 * Specify a Download to a running chunk
	 */
	 void setDownload(int64_t, Download*, bool, bool streaming);

	/**
     * Convert all unfinished chunks range data to a string, eg. "0 5000 10000 30000 "
     */
	string getFreeChunksString();

	/**
     * Convert all verified chunks range data to a string, eg. "0 65536 "
     */
	string getVerifiedBlocksString();

	/**
     * Mark a chunk range as download finished
     */
	int addChunkPos(int64_t, int64_t, size_t&);


	void getAllChunks(vector<int64_t>& v, int type);
	//void getDownloadedChunks(vector<pair<pair<uint64_t, uint64_t>, bool> v);

	/**
	 * Add the specified partial info to the aversion vector
	 */
	void addAversion(const PartsInfo& partialInfo, const PartsInfo& runInfo);

	/**
	 * Send a Partial Search result containing all sorts of information about your parts.
	 */
	void sendPSR(const PartialPeer& partialPeer, bool wantResponse);

	/**
	 * Add a partial peer to the list to notify of completed chunks.
	 */
	void storePartialPeer(const PartialPeer& partialPeer);

	/**
	 * Send partial search results to all stored partial peers: used after a new chunk is started.
	 * Note that this method may produce a large amount of UDP traffic, but it's worth it.
	 */
	void sendPSRToStoredPeers();

    int64_t getDownloadedSize() const
    {
        return downloadedSize;
    }

    int64_t getVerifiedSize() const
    {
        return verifiedSize;
    }

	/**
     * Perform last verifying against TigerTree
     */
	bool doLastVerify(const TigerTree& aTree, const string& tempTargetName);

	void markVerifiedBlock(uint16_t start, uint16_t end);

	/**
	 * Is specified parts needed by this download?
	 */
	bool isSource(const PartsInfo& partsInfo);

	/**
     * Is specified chunk start verified? If true, the chunk len is set.
	 */
	bool isVerified(int64_t startPos, int64_t& len);

	/**
	 * Verify a block	
	 * Note: if the block is not finished, true is returned
 	 */	
 	bool verifyBlock(int64_t anyPos, const TigerTree& aTree, const string& tempTargetName);

	/**
	 * Tests if the specified range was downloaded
	 */
	bool isRangeDownloaded(int64_t begin, int64_t end);

	size_t getBlockSize() const {
		return tthBlockSize;
	}

	int64_t getMinChunkSize() const {
		return minChunkSize;
	}

	int64_t getFileSize() const {
		return fileSize;
	}

private:
	const TTHValue m_tth;
 	/**
	 * Debug
	 */

	inline void selfCheck() const;

	typedef HASH_MAP_X(TTHValue*, Ptr, TTHValue::PtrHash, TTHValue::PtrHash, TTHValue::PtrLess) tthMap;
	
	static tthMap vecAllFileChunksInfo;
	static CriticalSection hMutexMapList;

	CriticalSection cs;
	
	// chunk start position must be unique in both waiting and running
	Chunk::Map waiting;
	Chunk::Map running;

    BlockMap verifiedBlocks;
    BlockVector blockAversion;				// indexed by tth block index, holds the aversion of that block

	int64_t fileSize;
    int64_t downloadedSize;
    int64_t verifiedSize;
	int64_t minChunkSize;					// it'll be doubled when last verifying fail

    size_t	tthBlockSize;					// tiger tree hash block size

	bool verify(const unsigned char* data, int64_t start, int64_t end, const TigerTree& aTree);

	PartialPeer::List partialPeers;			// vector of all the peers to send PSRs to

	// for debug purpose
	void dumpVerifiedBlocks();
	void dumpChunks(const char* context) const;
};

string GetPartsString(const PartsInfo& partsInfo);

#endif