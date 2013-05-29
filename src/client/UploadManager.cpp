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

#include "stdinc.h"
#include "DCPlusPlus.h"

//[-]PPA [Doxygen 1.5.1] #include "UploadManager.h"

#include "ConnectionManager.h"
//[-]PPA [Doxygen 1.5.1] #include "LogManager.h"
#include "ShareManager.h"
//[-]PPA [Doxygen 1.5.1] #include "ClientManager.h"
#include "FilteredFile.h"
//[-]PPA [Doxygen 1.5.1] #include "ZUtils.h"
#include "ResourceManager.h"
//[-]PPA [Doxygen 1.5.1] #include "HashManager.h"
#include "AdcCommand.h"
#include "FavoriteManager.h"
#include "CryptoManager.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "SharedFileStream.h"
#include "PGLoader.h"
#include "peers/PiwikTracker.h"

static const string UPLOAD_AREA = "Uploads";

Upload::Upload(UserConnection& conn) : Transfer(conn), stream(0) { 
	conn.setUpload(this);
}

Upload::~Upload() { 
	getUserConnection().setUpload(0);
	delete stream; 
}

void Upload::getParams(const UserConnection& aSource, StringMap& params) {
	Transfer::getParams(aSource, params);
	params["source"] = getSourceFile();
}

UploadManager::UploadManager() throw() : running(0), extra(0), lastGrant(0), mUploadLimit(0), 
	mBytesSent(0), mBytesSpokenFor(0), mCycleTime(0), mByteSlice(0), mThrottleEnable(BOOLSETTING(THROTTLE_ENABLE)), 
	m_iHighSpeedStartTick(0), isFireball(false), isFileServer(false) {	
	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	throttleZeroCounters();
}

UploadManager::~UploadManager() throw() {
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	{
		Lock l(cs);
		for(UploadQueueItem::UserMapIter ii = waitingUsers.begin(); ii != waitingUsers.end(); ++ii) {
			for(UploadQueueItem::Iter i = ii->second.begin(); i != ii->second.end(); ++i) {
				(*i)->dec();
			}
		}
		waitingUsers.clear();
	}

	while(true) {
		{
			Lock l(cs);
			if(uploads.empty())
				break;
		}
		Thread::sleep(100);
	}
}

// !SMT!-S
bool UploadManager::handleBan(UserConnection& aSource, bool forceBan, bool noChecks)
{
        const UserPtr& user = aSource.getUser();
        if (!ClientManager::getInstance()->getOnlineUser(user)) { // if not online, cheat (connection without hub)
                aSource.disconnect();
                return true;
        }
        bool banByRules = user->hasAutoBan();
        if (banByRules) {
                FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(aSource.getHubUrl());
                if (hub && hub->getExclChecks())
                        banByRules = false;
        }
        if (!forceBan && (noChecks || !banByRules)) return false;

        banmsg_t msg;
        msg.share = (int)(user->getBytesShared() / (1024*1048576));
        msg.slots = user->getCountSlots();
        msg.limit = user->getLimit();
        msg.min_share = min(min((int)(ShareManager::getInstance()->getShareSize()/(1024 * 1048576)),20),SETTING(BAN_SHARE));
        msg.min_slots = min(min(SETTING(SLOTS)/2,15),SETTING(BAN_SLOTS)); 
        msg.min_limit = min(min(SETTING(MAX_UPLOAD_SPEED_LIMIT_NORMAL)/2,60),SETTING(BAN_LIMIT));
				                
		
//2) "автобан" по шаре максимум ограничить размером "своя шара" (но не более 20 гиг)
//3) "автобан" по слотам максимум ограничить размером "сколько слотов у меня"/2 (но не более 15 слотов)
//4) "автобан" по лимиттеру максимум ограничить размером "лимиттер у меня"/2 (но не выше 60 кб/сек)

        // BAN for Slots:[1/5] Share:[17/200]Gb Limit:[30/50]kb/s Msg: share more files, please!
        string banstr = "BAN for";
        if (!forceBan) {
                if (msg.slots < msg.min_slots)
                        banstr = banstr + " Slots:[" + Util::toString(msg.slots) + "/" + Util::toString(msg.min_slots) + "]";
                if (msg.share < msg.min_share)
                        banstr = banstr + " Share:[" + Util::toString(msg.share) + "/" + Util::toString(msg.min_share) + "]Gb";
                if (msg.limit && msg.limit < msg.min_limit)
                        banstr = banstr + " Limit:[" + Util::toString(msg.limit) + "/" + Util::toString(msg.min_limit) + "]kb/s";
                banstr = banstr + " Msg: " + SETTING(BAN_MESSAGE);
        } else
		{
			banstr = "BAN forever";
		}
#ifdef _DEBUG
{
			banstr = "Log autoban:";
                if (msg.slots < msg.min_slots)
                        banstr = banstr + " Slots:[" + Util::toString(msg.slots) + "/" + Util::toString(msg.min_slots) + "]";
                if (msg.share < msg.min_share)
                        banstr = banstr + " Share:[" + Util::toString(msg.share) + "/" + Util::toString(msg.min_share) + "]Gb";
                if (msg.limit && msg.limit < msg.min_limit)
                        banstr = banstr + " Limit:[" + Util::toString(msg.limit) + "/" + Util::toString(msg.min_limit) + "]kb/s";
                banstr = banstr + " Msg: " + SETTING(BAN_MESSAGE);
				const string l_UserName = user->getFirstNick(); //[+]PPA
			    LogManager::getInstance()->message("User:" + l_UserName + " "+ banstr); //[+]PPA

		}
#endif
		bool sendStatus = aSource.isSet(UserConnection::FLAG_SUPPORTS_BANMSG);

		if (BOOLSETTING(BAN_FORCE_PM) || (!sendStatus && !BOOLSETTING(BAN_STEALTH)))
		{
                static uint64_t lastMsg = 0;
                msg.tick = TimerManager::getTick();
                if (msg.tick - lastMsg > 1000 && SETTING(BANMSG_PERIOD)) {
					    const string l_Key = user->getCID().toBase32();
                        BanMapIter t = lastBans.find(l_Key);
                        if (t == lastBans.end() || !t->second.same(msg) || (uint32_t)(msg.tick - t->second.tick) >= (uint32_t)SETTING(BANMSG_PERIOD)*60*1000) {
                                lastBans[l_Key] = msg;
                                lastMsg = msg.tick;
					            const string l_UserName = user->getFirstNick(); //[+]PPA
 				                LogManager::getInstance()->message("User:" + l_UserName + " "+ banstr); //[+]PPA
                                ClientManager::getInstance()->privateMessage(user, banstr, false);
                        }
                }
        }

        if (sendStatus && !BOOLSETTING(BAN_STEALTH)) aSource.error(banstr);

        if (BOOLSETTING(BAN_STEALTH)) aSource.maxedOut();
        return true;
}
// !SMT!-S
bool UploadManager::isBanReply(const UserPtr& user)
{
        BanMapIter t = lastBans.find(user->getCID().toBase32());
        if (t == lastBans.end()) return false;
        return (uint32_t)(TimerManager::getTick() - t->second.tick) < 2000;
}
bool UploadManager::hasUpload(UserConnection& aSource)
{
        const string l_srcip = aSource.getSocket()->getIp();
        //[+]necros
        for (Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
                Upload* u = *i;
				if(u && u->getUser())
				{
                const string l_ip = u->getUserConnection().getSocket()->getIp();
                if (abs(aSource.getUser()->getBytesShared() - u->getUser()->getBytesShared()) < 1024*1024*10
					&& l_srcip == l_ip)
                        return true;
				}
        }
        //[~]necros
        return false;
}

bool UploadManager::prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aStartPos, int64_t& aBytes, bool listRecursive) {
	if(aFile.empty() || aStartPos < 0 || aBytes < -1 || aBytes == 0) {
		aSource.fileNotAvail("Invalid request");
		return false;
	}
	
	if(aFile.find("TestSUR") != string::npos) {
		LogManager::getInstance()->message("User: " + aSource.getUser()->getFirstNick() + " (" + aSource.getRemoteIp() + ") testing me!");
	} 
	
	bool partialShare = false;

	InputStream* is = 0;
	int64_t start = 0;
	int64_t bytesLeft = 0;
	int64_t size = 0;

	bool userlist = (aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME);
	bool free = userlist;
	bool leaves = false;
	bool partList = false;

	// Hide Share Mod
	bool isInSharingHub = false; 

	if(aSource.getUser()) {
		isInSharingHub = !ClientManager::getInstance()->getSharingHub(aSource.getUser());
	}
	//Hide Share Mod

	string sourceFile;
	try {
		if(aType == Transfer::TYPE_FILE) {
			sourceFile = ShareManager::getInstance()->toReal(aFile, isInSharingHub);

			if(aFile == Transfer::USER_LIST_NAME) {
				// Unpack before sending...
				string bz2 = File(sourceFile, File::READ, File::OPEN).read();
				string xml;
				CryptoManager::getInstance()->decodeBZ2(reinterpret_cast<const uint8_t*>(bz2.data()), bz2.size(), xml);
				// Clear to save some memory...
				string().swap(bz2);
				is = new MemoryInputStream(xml);
				start = 0;
				bytesLeft = size = xml.size();
			} else {
				File* f = new File(sourceFile, File::READ, File::OPEN);

				start = aStartPos;
				size = f->getSize();
				bytesLeft = (aBytes == -1) ? size : aBytes;

				if(size < (start + bytesLeft)) {
					aSource.fileNotAvail();
					delete f;
					return false;
				}

				free = free || (size <= (int64_t)(SETTING(SET_MINISLOT_SIZE) * 1024) );

				f->setPos(start);

				is = f;
				if((start + bytesLeft) < size) {
					is = new LimitedInputStream<true>(is, aBytes);
				}
			}
		} else if(aType == Transfer::TYPE_TTHL) {
			//sourceFile = ShareManager::getInstance()->toReal(aFile, isInSharingHub);
			sourceFile = aFile;
			MemoryInputStream* mis = ShareManager::getInstance()->getTree(aFile);
			if(!mis) {
				aSource.fileNotAvail();
				return false;
			}

			start = 0;
			bytesLeft = size = mis->getSize();
			is = mis;
			leaves = true;
			free = true;
		} else if(aType == Transfer::TYPE_LIST) {
			// Partial file list
			MemoryInputStream* mis = ShareManager::getInstance()->generatePartialList(aFile, listRecursive);
			if(mis == NULL) {
				aSource.fileNotAvail();
				return false;
			}
			// Some old dc++ clients err here...
			aBytes = -1;
			start = 0;
			bytesLeft = size = mis->getSize();
	
			is = mis;
			free = true;
			partList = true;
		} else {
			aSource.fileNotAvail("Unknown file type");
			return false;
		}
	} catch(const ShareException& e) {
		// -- Added by RevConnect : Partial file sharing upload
		if(aFile.compare(0, 4, "TTH/") == 0) {

			TTHValue fileHash(aFile.substr(4));

			// find in download queue
			string target;
			string tempTarget;

            if(QueueManager::getInstance()->getTargetByRoot(fileHash, target, tempTarget)){
				if(aType == Transfer::TYPE_FILE) {
					sourceFile = tempTarget;
					// check start position and bytes
					FileChunksInfo::Ptr chunksInfo = FileChunksInfo::Get(&fileHash);
					if(chunksInfo && chunksInfo->isVerified(aStartPos, aBytes)){
						try{
							SharedFileStream* ss = new SharedFileStream(sourceFile, aStartPos);
							is = ss;
							start = aStartPos;
							size = chunksInfo->getFileSize();
							bytesLeft = (aBytes == -1) ? size : aBytes;

							if(size < (start + bytesLeft)) {
								aSource.fileNotAvail();
								delete is;
								return false;
							}

							if((aStartPos + bytesLeft) < size) {
								is = new LimitedInputStream<true>(is, aBytes);
							}

							partialShare = true;
							goto ok;
						}catch(const Exception&) {
							aSource.fileNotAvail();
							//aSource.disconnect();
							delete is;
							return false;
						}
					}else{
						// Hit this when user readd partial source without partial info
						//dcassert(0);
					}
				}
			// Share finished file
			}else{
				target = FinishedManager::getInstance()->getTarget(fileHash.toBase32());

				if(!target.empty() && Util::fileExists(target)){
					if(aType == Transfer::TYPE_FILE) {
						sourceFile = target;
						try{
							is = new SharedFileStream(sourceFile, aStartPos, 0, true);
							start = aStartPos;
							size = File::getSize(sourceFile);
							bytesLeft = (aBytes == -1) ? size : aBytes;

							if(size < (start + bytesLeft)) {
								aSource.fileNotAvail();
								delete is;
								return false;
							}

							if((aStartPos + bytesLeft) < size) {
								is = new LimitedInputStream<true>(is, aBytes);
							}

							partialShare = true;
							goto ok;
						}catch(const Exception&){
							aSource.fileNotAvail();
							delete is;
							return false;
						}
					}
				}
			}
		}
		aSource.fileNotAvail(e.getError());
		return false;
	} catch(const Exception& e) {
		LogManager::getInstance()->message(STRING(UNABLE_TO_SEND_FILE) + sourceFile + ": " + e.getError());
		aSource.fileNotAvail();
		return false;
	}

ok:

	Lock l(cs);


		bool hasReserved = (reservedSlots.find(aSource.getUser()) != reservedSlots.end());
		bool isFavorite = FavoriteManager::getInstance()->hasSlot(aSource.getUser());

        // !SMT!-S
        if (BOOLSETTING(EXTRASLOT_TO_DL) && DownloadManager::getInstance()->checkFileDownload(aSource.getUser()))
                hasReserved = true;

                // !SMT!-S
        if (!userlist && handleBan(aSource, FavoriteManager::getInstance()->hasBan(aSource.getUser()), isFavorite || hasReserved)) {
                delete is;
                addFailedUpload(aSource.getUser(), sourceFile, aStartPos, size);
                aSource.disconnect();
                return false;
                }

        bool extraSlot = false;
        if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) 
		  {
            if(!(hasReserved || isFavorite || getFreeSlots() > 0 || getAutoSlot()) || hasUpload(aSource)) 
			{ // !SMT!-S
			bool supportsFree = aSource.isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS);
                        bool allowedFree = aSource.isSet(UserConnection::FLAG_HASEXTRASLOT) || 
							/* aSource.isSet(UserConnection::FLAG_OP) || !SMT!-S */ 
							getFreeExtraSlots() > 0;
			if(free && supportsFree && allowedFree) {
				extraSlot = true;
			} else {
				delete is;
				aSource.maxedOut();
				addFailedUpload(aSource.getUser(), sourceFile, aStartPos, size);
				aSource.disconnect();
				return false;
			}
		}

		setLastGrant(GET_TICK());
	}
	clearUserFiles(aSource.getUser());

	bool resumed = false;
	for(Upload::List::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i) {
		Upload* up = *i;
		if(&aSource == &up->getUserConnection()) {
			delayUploads.erase(i);
			if(sourceFile != up->getSourceFile()) {
                                logUpload(up);
			} else {
				resumed = true;
			}
			dcdebug("Upload from %s removed on next chunk\n", up->getUserConnection().getUser()->getFirstNick().c_str());
			delete up;
			break;
		}
	}

	Upload* u = new Upload(aSource);
	u->setStream(is);
	if(aBytes == -1)
		u->setSize(size);
	else
		u->setSize(start + bytesLeft);
		
	if(u->getSize() != size)
		u->setFlag(Upload::FLAG_CHUNKED);

	u->setFileSize(size);
	u->setStartPos(start);
	u->setSourceFile(sourceFile);

	if(userlist)
		u->setFlag(Upload::FLAG_USER_LIST);
	if(leaves)
		u->setFlag(Upload::FLAG_TTH_LEAVES);
	if(partList)
		u->setFlag(Upload::FLAG_PARTIAL_LIST);
	if(partialShare)
		u->setFlag(Upload::FLAG_PARTIAL_SHARE);
	if(resumed)
		u->setFlag(Upload::FLAG_RESUMED);

	uploads.push_back(u);

	throttleSetup();
	if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
		if(extraSlot) {
			if(!aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource.setFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra++;
			}
		} else {
			if(aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource.unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra--;
			}
			aSource.setFlag(UserConnection::FLAG_HASSLOT);
			running++;
		}
	}

	return true;
}

int64_t UploadManager::getRunningAverage() {
	Lock l(cs);
	int64_t avg = 0;
	for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
		Upload* u = *i;
		avg += (int)u->getRunningAverage();
	}
	return avg;
}

bool UploadManager::getAutoSlot() {
	/** A 0 in settings means disable */
	if(SETTING(MIN_UPLOAD_SPEED) == 0)
		return false;
	/** Only grant one slot per 30 sec */
	if(GET_TICK() < getLastGrant() + 30*1000)
		return false;
	/** Grant if upload speed is less than the threshold speed */
	return getRunningAverage() < (SETTING(MIN_UPLOAD_SPEED)*1024);
}

void UploadManager::removeUpload(Upload* aUpload, bool delay) {
	Lock l(cs);
    uploads.erase_and_check(aUpload);
	throttleSetup();
	if(delay) {
		aUpload->setStart(GET_TICK());
		delayUploads.push_back(aUpload);
	} else {
		delete aUpload;
	}
}

void UploadManager::reserveSlot(const UserPtr& aUser, uint32_t aTime) {
        {
                Lock l(cs);
                reservedSlots[aUser] = GET_TICK() + aTime*1000;
                save(); // !SMT!-S
        }
        if(aUser->isOnline())
                ClientManager::getInstance()->connect(aUser);
        if (BOOLSETTING(SEND_SLOTGRANT_MSG)) // !SMT!-S
                ClientManager::getInstance()->privateMessage(aUser, "+me " + STRING(SLOT_GRANTED_MSG) + " " + Text::fromT(Util::formatSeconds(aTime)), false); // !SMT!-S
}

void UploadManager::unreserveSlot(const UserPtr& aUser) {
	SlotIter uis = reservedSlots.find(aUser);
	if(uis != reservedSlots.end())
		reservedSlots.erase(uis);
        save(); // !SMT!-S
        if (BOOLSETTING(SEND_SLOTGRANT_MSG)) // !SMT!-S
                ClientManager::getInstance()->privateMessage(aUser, "+me " + STRING(SLOT_REMOVED_MSG), false); // !SMT!-S
}

void UploadManager::on(UserConnectionListener::Get, UserConnection* aSource, const string& aFile, int64_t aResume) throw() {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM::onGet Bad state, ignoring\n");
		return;
	}
	
	int64_t bytes = -1;
	if(prepareFile(*aSource, Transfer::TYPE_FILE, Util::toAdcFile(aFile), aResume, bytes)) {
		aSource->setState(UserConnection::STATE_SEND);
		aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
	}
}

void UploadManager::on(UserConnectionListener::Send, UserConnection* aSource) throw() {
	if(aSource->getState() != UserConnection::STATE_SEND) {
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}

	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	u->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_RUNNING);
	aSource->transmitFile(u->getStream());
	fire(UploadManagerListener::Starting(), u);
}

void UploadManager::on(AdcCommand::GET, UserConnection* aSource, const AdcCommand& c) throw() {
	int64_t aBytes = Util::toInt64(c.getParam(3));
	int64_t aStartPos = Util::toInt64(c.getParam(2));
	const string& fname = c.getParam(1);
	const string& type = c.getParam(0);

	if(prepareFile(*aSource, type, fname, aStartPos, aBytes, c.hasFlag("RE", 4))) {
		Upload* u = aSource->getUpload();
		dcassert(u != NULL);

		AdcCommand cmd(AdcCommand::CMD_SND);
		cmd.addParam(type).addParam(fname)
			.addParam(Util::toString(u->getPos()))
			.addParam(Util::toString(u->getSize() - u->getPos()));

		if(c.hasFlag("ZL", 4)) {
			u->setStream(new FilteredInputStream<ZFilter, true>(u->getStream()));
			u->setFlag(Upload::FLAG_ZUPLOAD);
			cmd.addParam("ZL1");
		}

		aSource->send(cmd);

		u->setStart(GET_TICK());
		aSource->setState(UserConnection::STATE_RUNNING);
		aSource->transmitFile(u->getStream());
		fire(UploadManagerListener::Starting(), u);
	}
}

void UploadManager::on(UserConnectionListener::BytesSent, UserConnection* aSource, size_t aBytes, size_t aActual) throw() {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	u->addPos(aBytes, aActual);
	throttleBytesTransferred(aActual);
}

void UploadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	Upload* u = aSource->getUpload();

	if(u) {
		fire(UploadManagerListener::Failed(), u, aError);
		//2piwik
		PiwikTracker::varsMap p;
		p["TTH"] = u->getTTH().toBase32();
		p["size"] = Util::toString(u->getSize());
		p["time"] = Util::toString((GET_TICK() - u->getStart()) / 1000);
		p["bytes"] = Util::toString(u->getActual());
		PiwikTracker::getInstance()->trackAction("upload/failed", &p);


		dcdebug("UM::onFailed: Removing upload from %s\n", aSource->getUser()->getFirstNick().c_str());
		removeUpload(u);
	}

	removeConnection(aSource);
}

void UploadManager::on(UserConnectionListener::TransmitDone, UserConnection* aSource) throw() {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	aSource->setState(UserConnection::STATE_GET);

	if(!u->isSet(Upload::FLAG_CHUNKED)) {
                logUpload(u);
		removeUpload(u);
	} else {
		removeUpload(u, true);
	}
}

void UploadManager::logUpload(Upload* u) {
	if(BOOLSETTING(LOG_UPLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !u->isSet(Upload::FLAG_USER_LIST)) && 
		!u->isSet(Upload::FLAG_TTH_LEAVES)) {
		StringMap params;
		u->getParams(u->getUserConnection(), params);
		LOG(LogManager::UPLOAD, params);
	}
	fire(UploadManagerListener::Complete(), u);
	//2piwik
	PiwikTracker::varsMap p;
	p["TTH"] = u->getTTH().toBase32();
	p["size"] = Util::toString(u->getSize());
	p["time"] = Util::toString((GET_TICK() - u->getStart()) / 1000);
	p["bytes"] = Util::toString(u->getActual());
	PiwikTracker::getInstance()->trackAction("upload/complete", &p);
}

void UploadManager::addFailedUpload(const UserPtr& User, const string& file, int64_t pos, int64_t size) {
	uint32_t itime = GET_TIME();
	bool found = false;
	UploadQueueItem::UserMap::iterator j = waitingUsers.find(User);
	if(j != waitingUsers.end()) {
		for(UploadQueueItem::Iter i = j->second.begin(); i != j->second.end(); ++i) {
			if((*i)->File == file) {
				(*i)->pos = pos;
				found = true;
				break;
			}
		}
	}
	if(found == false) {
		UploadQueueItem* qi = new UploadQueueItem(User, file, pos, size, itime);
		//UploadQueueItem::UserMap::iterator i = waitingUsers.find(User);
		if(j == waitingUsers.end()) {
				UploadQueueItem::List l;
				l.push_back(qi);
				waitingUsers.insert(make_pair(User, l));
			} else {
			j->second.push_back(qi);
		}
		fire(UploadManagerListener::QueueAdd(), qi);
	}
}

void UploadManager::clearUserFiles(const UserPtr& source) {
	UploadQueueItem::UserMap::iterator ii = waitingUsers.find(source);
	if(ii != waitingUsers.end()) {
		for(UploadQueueItem::Iter i = ii->second.begin(); i != ii->second.end(); ++i) {
			fire(UploadManagerListener::QueueItemRemove(), (*i));
			(*i)->dec();
		}
		waitingUsers.erase(ii);
		fire(UploadManagerListener::QueueRemove(), source);
	}
}

UploadQueueItem::UserMap UploadManager::getWaitingUsers() {
	Lock l(cs);
	return waitingUsers;
}

void UploadManager::removeConnection(UserConnection* aSource) {
	dcassert(aSource->getUpload() == NULL);
	aSource->removeListener(this);
	if(aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		running--;
		aSource->unsetFlag(UserConnection::FLAG_HASSLOT);

		UploadQueueItem::UserMap u;
		{
			Lock l(cs);
			u = waitingUsers;
		}

		int freeSlots = getFreeSlots()*2;
		for(UploadQueueItem::UserMapIter i = u.begin(); i != u.end(); ++i) {
			UserPtr aUser = i->first;
			if(aUser->isOnline()) {
				ClientManager::getInstance()->connect(aUser);
				freeSlots--;
			}
			if(freeSlots == 0) break;
		}
	} 
	if(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
		extra--;
		aSource->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
	}
}

void UploadManager::addConnection(UserConnection::Ptr conn) {
#ifdef PPA_INCLUDE_IPFILTER
		if (PGLoader::getInstance()->getIPBlockBool(conn->getRemoteIp())) 
		{
			conn->error("Your IP is Blocked!");
			conn->getUser()->setFlag(User::PG_BLOCK);
			LogManager::getInstance()->message("IPFilter: Blocked incoming connection to " + conn->getRemoteIp());
			QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_REMOVED);
			removeConnection(conn);
			return;
		}
#endif

#ifdef PPA_INCLUDE_PG
	if(SETTING(PG_ENABLE) && SETTING(PG_UP) && !conn->isSet(UserConnection::FLAG_OP) && PGLoader::getInstance()->notAbused()) {
		string company = PGLoader::getInstance()->getIPBlock(conn->getRemoteIp());
		if (!company.empty()) {
			conn->error("Your IP is Blocked! ("+company+")[PeerGuardian Plugin 1.0 Authror: AluM]");
			conn->getUser()->setFlag(User::PG_BLOCK);
			if(SETTING(PG_LOG)) {
				LogManager::getInstance()->message("PGPlugin: Blocked incoming connection to " + conn->getRemoteIp() + " (" + company + ")");
			}
			QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_REMOVED);
			removeConnection(conn);
			return;
		}
	}
#endif
	conn->addListener(this);
	conn->setState(UserConnection::STATE_GET);
}

void UploadManager::testSlotTimeout()
        {
                uint64_t current = GET_TICK(); 
                for(SlotIter j = reservedSlots.begin(); j != reservedSlots.end();) {
                        if(j->second < current) { // !SMT!-S
                                reservedSlots.erase(j++);
                        } else {
                                ++j;
                        }
                }
}

void UploadManager::on(TimerManagerListener::Minute, uint32_t /*aTick*/) throw() {
        UserList disconnects;
        {
                Lock l(cs);
                testSlotTimeout();

                if( BOOLSETTING(AUTO_KICK) ) {
                        for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i) {
                                Upload* u = *i;
                                if(u->getUser()->isOnline()) {
                                        u->unsetFlag(Upload::FLAG_PENDING_KICK);
                                        continue;
                                }

				if(u->isSet(Upload::FLAG_PENDING_KICK)) {
						disconnects.push_back(u->getUser());
						continue;
				}

				if(BOOLSETTING(AUTO_KICK_NO_FAVS) && FavoriteManager::getInstance()->isFavoriteUser(u->getUser())) {
					continue;
				}

				u->setFlag(Upload::FLAG_PENDING_KICK);
			}
		}
	}
		
	for(UserList::const_iterator i = disconnects.begin(); i != disconnects.end(); ++i) {
		LogManager::getInstance()->message(STRING(DISCONNECTED_USER) + (*i)->getFirstNick());
		ConnectionManager::getInstance()->disconnect(*i, false);
	}
}

void UploadManager::on(GetListLength, UserConnection* conn) throw() { 
	conn->listLen("42");
}

void UploadManager::on(AdcCommand::GFI, UserConnection* aSource, const AdcCommand& c) throw() {
	if(c.getParameters().size() < 2) {
		aSource->send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_PROTOCOL_GENERIC, "Missing parameters"));
		return;
	}

	const string& type = c.getParam(0);
	const string& ident = c.getParam(1);

	if(type == Transfer::TYPE_FILE) {
		try {
			aSource->send(ShareManager::getInstance()->getFileInfo(ident));
		} catch(const ShareException&) {
			aSource->fileNotAvail();
		}
	} else {
		aSource->fileNotAvail();
	}
}

// TimerManagerListener
void UploadManager::on(TimerManagerListener::Second, uint32_t aTick) throw() {
	{
		Lock l(cs);
		throttleSetup();
		throttleZeroCounters();

		if((aTick / 1000) % 10 == 0) {
			for(Upload::List::iterator i = delayUploads.begin(); i != delayUploads.end();) {
				Upload* u = *i;
				if((aTick - u->getStart()) > 15000) {
                                        logUpload(u);
					delete u;
					delayUploads.erase(i);
					i = delayUploads.begin();
				} else i++;
			}
		}

		if(uploads.size() > 0)
			fire(UploadManagerListener::Tick(), uploads);

		fire(UploadManagerListener::QueueUpdate());
	}
	if(!isFireball) {
		if(getRunningAverage() >= 102400) {
			if (m_iHighSpeedStartTick > 0) {
				if ((aTick - m_iHighSpeedStartTick) > 60000) {
					isFireball = true;
					ClientManager::getInstance()->infoUpdated(true);
					return;
				}
			} else {
				m_iHighSpeedStartTick = aTick;
			}
		} else {
			m_iHighSpeedStartTick = 0;
		}

		if(!isFileServer) {
			if(	(Util::getUptime() > 7200) && // > 2 hours uptime
				(Socket::getTotalUp() > 209715200) && // > 200 MB uploaded
				(ShareManager::getInstance()->getSharedSize() > 2147483648)) { // > 2 GB shared
					isFileServer = true;
					ClientManager::getInstance()->infoUpdated(true);
			}
		}
	}
}

void UploadManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
	if(!aUser->isOnline()) {
		Lock l(cs);
		clearUserFiles(aUser);
	}
}

size_t UploadManager::throttleGetSlice()  {
	if (mThrottleEnable) {
		size_t left = mUploadLimit - mBytesSpokenFor;
		if (left > 0) {
			if (left > 2*mByteSlice) {
				mBytesSpokenFor += mByteSlice;
				return mByteSlice;
			} else {
				mBytesSpokenFor += left;
				return left;
			}
		} else {
			return 16; // must send > 0 bytes or threadSendFile thinks the transfer is complete
		}
	} else {
		return (size_t)-1;
	}
}

size_t UploadManager::throttleCycleTime() {
	if (mThrottleEnable)
		return mCycleTime;
	return 0;
}

void UploadManager::throttleZeroCounters()  {
	if (mThrottleEnable) {
		mBytesSpokenFor = 0;
		mBytesSent = 0;
	}
}

void UploadManager::throttleBytesTransferred(uint32_t i)  {
	mBytesSent += i;
}

void UploadManager::throttleSetup() {
// called once a second, plus when uploads start
// from the constructor to BufferedSocket
	unsigned int num_transfers = uploads.size();
	mUploadLimit = SETTING(MAX_UPLOAD_SPEED_LIMIT) * 1024;
	mThrottleEnable = BOOLSETTING(THROTTLE_ENABLE) && (mUploadLimit > 0) && (num_transfers > 0);
	if (mThrottleEnable) {
		size_t inbufSize = SETTING(SOCKET_OUT_BUFFER);	
		if (mUploadLimit <= (inbufSize * 10 * num_transfers)) {
			mByteSlice = mUploadLimit / (5 * num_transfers);
			if (mByteSlice > inbufSize)
				mByteSlice = inbufSize;
			mCycleTime = 100;
		} else {
			mByteSlice = inbufSize;
			mCycleTime = 1000 * inbufSize / mUploadLimit;
		}
	}
}

/**
 * Abort upload of specific file
 */
void UploadManager::abortUpload(const string& aFile, bool waiting){
	bool nowait = true;

	{
		Lock l(cs);

		for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i){
			Upload::Ptr u = (*i);

			if(u->getSourceFile() == aFile){
				u->getUserConnection().disconnect(true);
				nowait = false;
			}
		}
	}
	
	if(nowait) return;
	if(!waiting) return;
	
	for(int i = 0; i < 20 && nowait == false; i++){
		Thread::sleep(250);
		{
			Lock l(cs);

			nowait = true;
			for(Upload::Iter i = uploads.begin(); i != uploads.end(); ++i){
				Upload::Ptr u = (*i);

				if(u->getSourceFile() == aFile){
					dcdebug("upload %s is not removed\n", aFile.c_str());
					nowait = false;
					break;
				}
			}
		}
	}
	
	if(!nowait)
		dcdebug("abort upload timeout %s\n", aFile.c_str());
}

// !SMT!-S
time_t UploadManager::getReservedSlotTime(const UserPtr& aUser) const {
        Lock l(cs);
        SlotIterC j = reservedSlots.find(aUser);
        return (j == reservedSlots.end())? 0 : j->second;
}

// !SMT!-S
void UploadManager::save() {
        Lock l(cs);
        try {
                SimpleXML xml;
                xml.addTag("ExtraSlots");
                xml.stepIn();
                for(SlotIterC i = reservedSlots.begin(); i != reservedSlots.end(); ++i) {
                        xml.addTag("ExtraSlot");
                        xml.addChildAttrib("UserCID", i->first->getCID().toBase32());
                        xml.addChildAttrib("SlotTimeout", i->second);
                }
                xml.stepOut();
                string fname = getConfigFile();

                File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
                f.write(SimpleXML::utf8Header);
                f.write(xml.toXML());
                f.close();
                File::deleteFile(fname);
                File::renameFile(fname + ".tmp", fname);
        } catch(const Exception& e) {
                dcdebug("UploadManager::save: %s\n", e.getError().c_str());
        }
}

// !SMT!-S
void UploadManager::load() {
        try {
                Lock l(cs);
                SimpleXML xml;
                xml.fromXML(File(getConfigFile(), File::READ, File::OPEN).read());

                if(xml.findChild("ExtraSlots")) {
                        xml.stepIn();
                        while(xml.findChild("ExtraSlot")) {
                                CID cid(xml.getChildAttrib("UserCID"));
                                time_t timeout = Util::toInt(xml.getChildAttrib("SlotTimeout"));
                                reservedSlots[ClientManager::getInstance()->getUser(cid)] = timeout;
                        }
                        xml.stepOut();
                }
                testSlotTimeout();
        } catch(const Exception& e) {
                dcdebug("UploadManager::load: %s\n", e.getError().c_str());
        }
}

// !SMT!-UI
UploadQueueItem::UploadQueueItem(const UserPtr& u, const string& file, int64_t p, int64_t sz, uint32_t itime) :
                User(u), File(file), pos(p), size(sz), iTime(itime), icon(0)
{
	{
		ClientManager::LockInstance l_lockInstance;
        OnlineUser* ou = ClientManager::getInstance()->getOnlineUser(User);
        if (ou) {
                share = ou->getIdentity().getBytesShared();
                slots = ou->getIdentity().getSlots();
                ip = ou->getIdentity().getIp();
#ifdef PPA_INCLUDE_DNS
                dns = Socket::nslookup(ip);
#endif
                country = Util::getIpCountry(ip);
                flagImage = -1;
        }
		else 
		{
                share = 0;
                slots = 0;
                ip = Util::emptyString;
#ifdef PPA_INCLUDE_DNS
                dns = Util::emptyString;
#endif
                country = Util::emptyString;
                flagImage = 0;
        }
        
	}
    inc();
}

/**
 * @file
 * $Id: UploadManager.cpp,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
