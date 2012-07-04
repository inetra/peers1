/*
 * ApexDC speedmod (c) SMT 2007
 */
#include "stdafx.h"
#include "../client/ShareManager.h"
#include "../client/TimerManager.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"
#include "../client/UploadManager.h"
#include "../client/HashManager.h"
#include "UserInfoBase.h"
#include "../peers/PrivateFrameFactory.h"
#include "LineDlg.h"
#include "../peers/HubFrameFactory.h"

void UserInfoBase::matchQueue() {
        try {
                QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
        } catch(const Exception& e) {
                LogManager::getInstance()->message(e.getError());
        }
}

void UserInfoBase::getUserResponses() {
        try {
                QueueManager::getInstance()->addTestSUR(user, false);
        } catch(const Exception& e) {
                LogManager::getInstance()->message(e.getError());
        }
}

void UserInfoBase::doReport() {
        ClientManager::getInstance()->reportUser(user);
}

void UserInfoBase::getList() {
        try {
                QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
        } catch(const Exception& e) {
                LogManager::getInstance()->message(e.getError());
        }
}
void UserInfoBase::browseList() {
        if(getUser()->getCID().isZero())
                return;
        try {
                QueueManager::getInstance()->addPfs(user, "");
        } catch(const Exception& e) {
                LogManager::getInstance()->message(e.getError());
        }
}
void UserInfoBase::checkList() {
        try {
                QueueManager::getInstance()->addList(user, QueueItem::FLAG_CHECK_FILE_LIST);
        } catch(const Exception& e) {
                LogManager::getInstance()->message(e.getError());
        }
}
void UserInfoBase::addFav() {
        FavoriteManager::getInstance()->addFavoriteUser(user);
}
void UserInfoBase::pm() {
        PrivateFrameFactory::openWindow(user);
}
// !SMT!-UI
void UserInfoBase::addSummary()
{
        HCURSOR hcurPrev = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
        tstring userInfo;
        string ip;
		uint64_t l_share = 0;
		{
          ClientManager::LockInstance l_lockInstance;
        if (OnlineUser *ou = ClientManager::getInstance()->getOnlineUser(user)) {
                l_share = ou->getIdentity().getBytesShared();
                const int l_slots = ou->getIdentity().getSlots();
                const int l_limit = ou->getIdentity().getLimit();
                userInfo = userInfo + _T("Slots=") + Util::toStringW(l_slots) + _T(" Share=") + Util::formatBytesW(l_share);
                if (l_limit) 
					userInfo = userInfo + _T(" Limit=") + Util::toStringW(l_limit) + _T("kb/s");
        }
		}
        string dns;
#ifdef PPA_INCLUDE_DNS
         dns = Socket::nslookup(ip);
         if (ip == dns) 
			 dns = "no DNS";
         if (!dns.empty()) 
			 dns = " / " + dns;
#endif

        WinUtil::userSummaryMenu.InsertSeparatorLast(Text::toT(getUser()->getFirstNick()));
        WinUtil::userSummaryMenu.AppendMenu(MF_STRING | MF_DISABLED, IDC_NONE, userInfo.c_str());
        if (!ip.empty())
                WinUtil::userSummaryMenu.AppendMenu(MF_STRING | MF_DISABLED, IDC_NONE, (Text::toT("IP: " + ip + dns)).c_str());

        time_t slot = UploadManager::getInstance()->getReservedSlotTime(user);
        if (slot) {
                tstring note = TSTRING(EXTRA_SLOT_TIMEOUT) + _T(": ") + Util::formatSeconds((slot - GET_TICK())/1000);
                WinUtil::userSummaryMenu.AppendMenu(MF_STRING | MF_DISABLED, IDC_NONE, note.c_str());
        }

        WinUtil::userSummaryMenu.AppendMenu(MF_SEPARATOR);

        HubFrameFactory::addDupeUsersToSummaryMenu(l_share, ip);

        bool caption = false;
        UploadQueueItem::UserMap users = UploadManager::getInstance()->getWaitingUsers();
        for(UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
                if (uit->first != getUser()) continue;
                for(UploadQueueItem::Iter i = uit->second.begin(); i != uit->second.end(); ++i) {
                        if (!caption) {
                                WinUtil::userSummaryMenu.InsertSeparatorLast(TSTRING(USER_WAIT_MENU));
                                caption = true;
                        }
                        (*i)->update();
                        tstring note =
                            tstring(_T("[")) +
                            Text::toT(Util::toString((double)(*i)->pos*100.0/(double)(*i)->size)) +
                            tstring(_T("% ")) +
                            Util::formatSeconds(GET_TIME()-(*i)->iTime) +
                            tstring(_T("]\x09")) +
                            (*i)->getText(UploadQueueItem::COLUMN_FILE);
                        WinUtil::userSummaryMenu.AppendMenu(MF_STRING | MF_DISABLED, IDC_NONE, note.c_str());
                }
        }
        caption = false;
        const QueueItem::StringMap& downloads = QueueManager::getInstance()->lockQueue();
        for(QueueItem::StringMap::const_iterator j = downloads.begin(); j != downloads.end(); ++j) {
                QueueItem* aQI = j->second;
                bool src = aQI->isSource(getUser()), badsrc = false;
                if (!src) badsrc = aQI->isBadSource(getUser());
                if (src || badsrc) {
                        if (!caption) {
                                WinUtil::userSummaryMenu.InsertSeparatorLast(TSTRING(NEED_USER_FILES_MENU));
                                caption = true;
                        }
                        tstring note = Text::toT(aQI->getTarget());
                        if (aQI->getSize() > 0)
                                note = note + tstring(_T("\x09(")) + Text::toT(Util::toString((double)aQI->getDownloadedBytes()*100.0/(double)aQI->getSize())) + tstring(_T("%)"));
                        UINT flags = MF_STRING | MF_DISABLED;
                        if (badsrc) flags |= MF_GRAYED;
                        WinUtil::userSummaryMenu.AppendMenu(flags, IDC_NONE, note.c_str());
                }
        }
        QueueManager::getInstance()->unlockQueue();

        ::SetCursor(hcurPrev);
}
// !SMT!-S
void UserInfoBase::pm_msg(void *param) {
   const tstring *pmessage = (const tstring*)param;
   if (*pmessage != _T("\x01"))
      PrivateFrameFactory::openWindow(getUser(), *pmessage);
}
// !SMT!-S
const tstring UserInfoBase::getBroadcastPrivateMessage() {
   static tstring deftext = _T("");
   LineDlg dlg;
   dlg.description = TSTRING(PRIVATE_MESSAGE);
   dlg.title = TSTRING(SEND_TO_ALL_USERS);
   dlg.line = deftext;
   if (dlg.DoModal() == IDOK) {
      deftext = dlg.line;
      return deftext;
   } else // cancel
      return _T("\x01");
}
uint32_t UserInfoBase::inputSlotTime() {
   static tstring deftext = _T("00:30");
   LineDlg dlg;
   dlg.description = TSTRING(EXTRA_SLOT_TIME_FORMAT);
   dlg.title = TSTRING(EXTRA_SLOT_TIMEOUT);
   dlg.line = deftext;
   if (dlg.DoModal() == IDOK) {
      deftext = dlg.line;
      int n = 0;
      for (unsigned i = 0; i < deftext.length(); i++)
         if (deftext[i] == ':') n++;
      int d,h,m;
      switch (n) {
         case 1:
            swscanf(deftext.c_str(), L"%d:%d", &h, &m);
            return (h*3600+m*60);
         case 2:
            swscanf(deftext.c_str(), L"%d:%d:%d", &d, &h, &m);
            return (d*3600*24+h*3600+m*60);
         default:
            ::MessageBox(GetForegroundWindow(), CTSTRING(INVALID_TIME_FORMAT), CTSTRING(ERRORS), MB_OK | MB_ICONERROR);
            return 0;
      }
   } // else // cancel
   return 0;
}
void UserInfoBase::grant() {
        UploadManager::getInstance()->reserveSlot(user, 600);
}
void UserInfoBase::removeAll() {
        QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
}
void UserInfoBase::grantSlotHour() {
        UploadManager::getInstance()->reserveSlot(user, 3600);
}
void UserInfoBase::grantSlotDay() {
        UploadManager::getInstance()->reserveSlot(user, 24*3600);
}
void UserInfoBase::grantSlotWeek() {
        UploadManager::getInstance()->reserveSlot(user, 7*24*3600);
}
void UserInfoBase::ungrantSlot() {
        UploadManager::getInstance()->unreserveSlot(user);
}
// !SMT!-UI
void UserInfoBase::grantSlotPeriod(void *period)
{
        if (period)
                UploadManager::getInstance()->reserveSlot(user, (uint32_t)period);
}
