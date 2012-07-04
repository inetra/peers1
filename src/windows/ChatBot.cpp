/*
 * ApexDC speedmod (c) SMT 2007
 */

#include "stdafx.h"
#include "../client/ClientManager.h"
#include "../client/FavoriteUser.h"
#include "../client/FavoriteManager.h"
#include "../client/UploadManager.h"
#include "ChatBot.h"
#include "../peers/PrivateFrameFactory.h"
#include "../peers/PeersVersion.h"

ChatBot::ChatBot() : hDll(NULL), qrycount(0)
{
        init.apiVersion = 2;
        init.appName = APPNAME;
        init.appVersion = VERSIONSTRING;
        init.botId = 0;
        init.botVersion = NULL;
        init.SendMessage = botSendMessage;
        init.RecvMessage = NULL;
        init.SendMessage2 = botSendMessage2;
        init.RecvMessage2 = NULL;
        init.QueryInfo = botQueryInfo_rc;
        init.FreeInfo = botFreeInfo;

        hDll = ::LoadLibrary(_T("ChatBot.dll"));
        if (hDll) {
                BotInit::tInit initproc = (BotInit::tInit)GetProcAddress(hDll, "init");
                if (initproc) {
                        if (!initproc(&init))
                                init.RecvMessage = NULL;
                }
        }
}

ChatBot::~ChatBot() {
        if (hDll) ::FreeLibrary(hDll);
}

void ChatBot::botSendMessage(const WCHAR *params, const WCHAR *message)
{
        const WCHAR *cid = wcsstr(params, L"CID=");
        if (!cid) return;
        botSendMessage2(BotInit::SEND_PM, cid+4, message, (wcslen(message)+1)*sizeof(WCHAR));
}

bool ChatBot::botSendMessage2(int msgid, const WCHAR* objid, const void *param, unsigned /*paramsize*/)
{
        if (msgid == BotInit::SEND_CM) {
                Client* client = ClientManager::getInstance()->findClient(Text::fromT((WCHAR*)objid));
                if (client == NULL) return false;
                client->hubMessage(Text::fromT((WCHAR*)param));
                return true;
        }

        if (msgid == BotInit::DL_MAGNET) {
                // todo: fixme
                return false;
        }

        UserPtr user = ClientManager::getInstance()->getUser(CID(Text::fromT((WCHAR*)objid)));
        if (!user) return false;

        switch (msgid) {
        case BotInit::SEND_PM:
                ClientManager::getInstance()->privateMessage(user, Text::fromT((WCHAR*)param));
                return true;
        case BotInit::USER_CLOSE:
                return PrivateFrameFactory::closeUser(user);
        case BotInit::USER_IGNORE:
                FavoriteManager::getInstance()->addFavoriteUser(user);
                FavoriteManager::getInstance()->setIgnorePrivate(user, (param && ((DWORD)param==1) || *(DWORD*)param));
                return true;
        case BotInit::USER_BAN:
                FavoriteManager::getInstance()->addFavoriteUser(user);
                FavoriteManager::getInstance()->setUploadLimit(user, (param && ((DWORD)param==1) || *(DWORD*)param)? FavoriteUser::UL_BAN : FavoriteUser::UL_NONE);
                return true;
        case BotInit::USER_SLOT:
                if (param)
                        UploadManager::getInstance()->reserveSlot(user, (uint32_t)(param? *(uint32_t*)param : 0));
                else
                        UploadManager::getInstance()->unreserveSlot(user);
                return true;
        }
        return false;
}

void* ChatBot::botQueryInfo_rc(int qryid, const WCHAR* objid, const void *param, unsigned paramsize)
{
        ChatBot *self = ChatBot::getInstance();
        void* res = self->botQueryInfo(qryid, objid, param, paramsize);
        if (res) self->qrycount++;
        return res;
}

void ChatBot::onUserAction(BotInit::CODES c, const UserPtr& aUser)
{
        if (!aUser) return; //[+]PPA
	    if (!init.RecvMessage2) return;
        try {
                init.RecvMessage2(c, Text::toT(aUser->getCID().toBase32()).c_str(), NULL, 0);
        } catch (Exception&) {
                externalFailure();
        }
}

void ChatBot::onHubAction(BotInit::CODES c, const string& hubUrl)
{
        if (!init.RecvMessage2) return;
        try {
                init.RecvMessage2(c, Text::toT(hubUrl).c_str(), NULL, 0);
        } catch (Exception&) {
                externalFailure();
        }
}

WCHAR* ChatBot::onQueryUserByCid(const WCHAR* cid)
{
        UserPtr user = ClientManager::getInstance()->getUser(CID(Text::fromT(cid)));
        if (!user) return NULL;

        ParamSet ps;
        ps.addVariable(L"CID", cid);
        ps.addVariable(L"NICK", Text::toT(user->getFirstNick()).c_str());

		{
		ClientManager::LockInstance l_instance;
        OnlineUser *ou = ClientManager::getInstance()->getOnlineUser(user);
        if (!ou) 
			return NULL;

        const Identity& id = ou->getIdentity();
        string ip = id.getIp();

        ps.addVariable(L"IP", Text::toT(ip).c_str());
#ifdef PPA_INCLUDE_DNS
        ps.addVariable(L"DNS", Text::toT(Socket::nslookup(ip)).c_str());
#endif
		ps.addVariable(L"DESC", Text::toT(id.getDescription()).c_str());
        ps.addVariable(L"OP", id.isOp()? L"1" : L"0");
        ps.addVariable(L"BOT", id.isBot()? L"1" : L"0");
        ps.addVariable(L"AWAY", user->isSet(User::AWAY)? L"1" : L"0");
        ps.addVariable(L"SLOTS", Text::toT(id.get("SL")).c_str());
        ps.addVariable(L"LIMIT", Text::toT(id.get("LI")).c_str());
        ps.addVariable(L"EXACTSHARE", Text::toT(id.get("SS")).c_str());
        ps.addVariable(L"SHARE", Util::formatBytesW(id.getBytesShared()).c_str());
        ps.addVariable(L"HUBURL", Text::toT(ou->getClient().getHubUrl()).c_str());
		}
        bool isFav = FavoriteManager::getInstance()->isFavoriteUser(user);
        ps.addVariable(L"ISFAV", isFav? L"1" : L"0");
        if (isFav) {
                ps.addVariable(L"FAVSLOT", FavoriteManager::getInstance()->hasSlot(user)? L"1" : L"0");
                ps.addVariable(L"FAVBAN", FavoriteManager::getInstance()->hasBan(user)? L"1" : L"0");
                ps.addVariable(L"FAVIGNORE", FavoriteManager::getInstance()->hasIgnore(user)? L"1" : L"0");
        }
        return ps.cutParams();
}

WCHAR* ChatBot::onQueryHubByUrl(const WCHAR* huburl)
{
        ParamSet ps;
	{
	ClientManager::LockInstance l_instance;
        Client* c = ClientManager::getInstance()->findClient(Text::fromT(huburl));
        if (!c) 
	    return NULL; 
        ps.addVariable(L"HUBURL", huburl);
        ps.addVariable(L"HUBNAME", Text::toT(c->getHubName()).c_str());
        ps.addVariable(L"HUBDESC", Text::toT(c->getHubDescription()).c_str());
        ps.addVariable(L"IP", Text::toT(c->getIp()).c_str());
        ps.addVariable(L"PORT", Text::toT(Util::toString(c->getPort())).c_str());
        }
        return ps.cutParams();
}

WCHAR* ChatBot::onQueryConnectedHubs()
{
        ParamSet ps;
	{
	ClientManager::LockInstance l_instance;
        Client::List& l = ClientManager::getInstance()->getClients();
        for (Client::Iter i = l.begin(); i != l.end(); ++i)
                if ((*i)->isConnected())
                        ps.addValue(Text::toT((*i)->getHubUrl()).c_str());
	}
        return ps.cutParams();
}

WCHAR* ChatBot::onQueryHubUsers(const WCHAR* /* huburl */)
{
        // todo: fixme
        return NULL;
}

WCHAR* ChatBot::onQueryRunningUploads(const WCHAR* /* cid */)
{
        // todo: fixme
        return NULL;
}

WCHAR* ChatBot::onQueryDownloads(const WCHAR* cid)
{
        ParamSet ps;
        UserPtr user = cid? ClientManager::getInstance()->getUser(CID(Text::fromT(cid))) : NULL;
        const QueueItem::StringMap& downloads = QueueManager::getInstance()->lockQueue();
        for(QueueItem::StringMap::const_iterator j = downloads.begin(); j != downloads.end(); ++j) {
                QueueItem* aQI = j->second;
                bool src = aQI->isSource(user), badsrc = false;
                if (!src) badsrc = aQI->isBadSource(user);
                if (user && !src && !badsrc) continue;
                ps.addVariable(L"FILENAME", Text::toT(aQI->getTarget()).c_str());
                ps.addVariable(L"FILESIZE", Util::toStringW(aQI->getSize()).c_str());
                ps.addVariable(L"DOWNLOADED", Util::toStringW(aQI->getDownloadedBytes()).c_str());
                ps.addVariable(L"ISBADSRC", badsrc? L"1":L"0");
                WCHAR *status = 0;
                if (aQI->getStatus() == QueueItem::STATUS_WAITING) status = L"WAIT";
                if (aQI->getStatus() == QueueItem::STATUS_RUNNING) status = L"RUN";
                if (status) ps.addVariable(L"STATUS", status);
                ps.addVariable(L"PRIORITY", Util::toStringW(aQI->getPriority()).c_str());
                ps.addValue();
        }
        QueueManager::getInstance()->unlockQueue();
        return ps.cutParams();
}

WCHAR* ChatBot::onQueryQueuedUploads(const WCHAR* cid)
{
        ParamSet ps;
        UserPtr user = cid? ClientManager::getInstance()->getUser(CID(Text::fromT(cid))) : NULL;
        UploadQueueItem::UserMap users = UploadManager::getInstance()->getWaitingUsers();
        for(UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
                if (user && uit->first != user) continue;
                for(UploadQueueItem::Iter i = uit->second.begin(); i != uit->second.end(); ++i) {
                        (*i)->update();
                        ps.addVariable(L"CID", Text::toT(uit->first->getCID().toBase32()).c_str());
                        ps.addVariable(L"FILENAME", Text::toT((*i)->File).c_str());
                        ps.addVariable(L"FILESIZE", Util::toStringW((*i)->size).c_str());
                        ps.addVariable(L"POS", Util::toStringW((*i)->pos).c_str());
                        ps.addVariable(L"TIME", Util::toStringW(uint32_t(GET_TIME() - (*i)->iTime)).c_str());
                        ps.addValue();
                }
        }
        return ps.cutParams();
}

void* ChatBot::botQueryInfo(int qryid, const WCHAR* objid, const void* /*param*/, unsigned /*paramsize*/)
{
        switch (qryid) {
        case BotInit::QUERY_USER_BY_CID:
                return onQueryUserByCid((const WCHAR*)objid);
        case BotInit::QUERY_HUB_BY_URL:
                return onQueryHubByUrl((const WCHAR*)objid);
        case BotInit::QUERY_CONNECTED_HUBS:
                return onQueryConnectedHubs();
        case BotInit::QUERY_HUB_USERS:
                return onQueryHubUsers((const WCHAR*)objid);
        case BotInit::QUERY_RUNNING_UPLOADS:
                return onQueryRunningUploads((const WCHAR*)objid);
        case BotInit::QUERY_QUEUED_UPLOADS:
                return onQueryQueuedUploads((const WCHAR*)objid);
        case BotInit::QUERY_DOWNLOADS:
                return onQueryDownloads((const WCHAR*)objid);
        case BotInit::QUERY_SELF:
        {
                string selfcid = ClientManager::getInstance()->getMe()->getCID().toBase32();
                int sz = (selfcid.length()+1) * sizeof(WCHAR);
                WCHAR* info = (WCHAR*)malloc(sz);
                memcpy(info, Text::toT(selfcid).c_str(), sz);
                return info;
        }
        }
        return NULL;
}

void  ChatBot::botFreeInfo(void *info)
{
        if (info) ChatBot::getInstance()->qrycount--;
        free(info);
}

void ChatBot::onMessage(const string& huburl, const UserPtr& /*msgFrom*/, const string& message)
{
        if (!init.RecvMessage2) return;

        try {
                tstring msg = Text::toT(message);
                init.RecvMessage2(BotInit::RECV_CM, Text::toT(huburl).c_str(), msg.c_str(), (msg.length()+1)*sizeof(WCHAR));
        } catch (Exception&) {
                externalFailure();
        }
}

void ChatBot::onMessage(const UserPtr& msgFrom, const tstring& message, bool newSession)
{
        tstring::size_type pos = message.find(_T("> "));
        if (pos == tstring::npos) return;
        tstring realmessage = message.substr(pos+2);

        if (init.RecvMessage)
                onMessageV1(msgFrom, realmessage, newSession);
        if (init.RecvMessage2)
                onMessageV2(msgFrom, realmessage, newSession);
}

void ChatBot::externalFailure()
{
        // stop further notifications
        init.RecvMessage = NULL;
        init.RecvMessage2 = NULL;
        ClientManager::getInstance()->privateMessage(ClientManager::getInstance()->getMe(), "ChatBot died!");
}

void ChatBot::onMessageV1(const UserPtr& msgFrom, const tstring& message, bool newSession)
{
        wstring cid = Text::toT(msgFrom->getCID().toBase32());
        ParamSet ps;
        ps.addVariable(L"CID", cid.c_str());
        ps.addVariable(L"NICK", Text::toT(msgFrom->getFirstNick()).c_str());

	{
	ClientManager::LockInstance l_instance;
        OnlineUser *ou = ClientManager::getInstance()->getOnlineUser(msgFrom);
        if (!ou) 
	    return; 
        const Identity& id = ou->getIdentity();
        const Identity& myId = ou->getClient().getMyIdentity();
        string ip = id.getIp();

        ps.addVariable(L"IP", Text::toT(ip).c_str());
#ifdef PPA_INCLUDE_DNS
        ps.addVariable(L"DNS", Text::toT(Socket::nslookup(ip)).c_str());
#endif
		ps.addVariable(L"DESC", Text::toT(id.getDescription()).c_str());
        ps.addVariable(L"OP", id.isOp()? L"1" : L"0");
        ps.addVariable(L"BOT", id.isBot()? L"1" : L"0");
        ps.addVariable(L"AWAY", msgFrom->isSet(User::AWAY)? L"1" : L"0");
        ps.addVariable(L"SLOTS", Text::toT(id.get("SL")).c_str());
        ps.addVariable(L"LIMIT", Text::toT(id.get("LI")).c_str());
        ps.addVariable(L"EXACTSHARE", Text::toT(id.get("SS")).c_str());
        ps.addVariable(L"SHARE", Util::formatBytesW(id.getBytesShared()).c_str());
        ps.addVariable(L"MYNICK", Text::toT(myId.getNick()).c_str());
        ps.addVariable(L"MYSLOTS", Text::toT(myId.get("SL")).c_str());
        ps.addVariable(L"MYLIMIT", Text::toT(myId.get("LI")).c_str());
        ps.addVariable(L"MYEXACTSHARE", Text::toT(myId.get("SS")).c_str());
        ps.addVariable(L"MYSHARE", Util::formatBytesW(myId.getBytesShared()).c_str());
        ps.addVariable(L"MYAWAY", Util::getAway()? L"1" : L"0");
        ps.addVariable(L"HUBURL", Text::toT(ou->getClient().getHubUrl()).c_str());
        ps.addVariable(L"HUBNAME", Text::toT(ou->getClient().getHubName()).c_str());
        ps.addVariable(L"HUBDESC", Text::toT(ou->getClient().getHubDescription()).c_str());

        bool isFav = FavoriteManager::getInstance()->isFavoriteUser(msgFrom);
        ps.addVariable(L"ISFAV", isFav? L"1" : L"0");
        if (isFav) {
                ps.addVariable(L"FAVSLOT", FavoriteManager::getInstance()->hasSlot(msgFrom)? L"1" : L"0");
                ps.addVariable(L"FAVBAN", FavoriteManager::getInstance()->hasBan(msgFrom)? L"1" : L"0");
                ps.addVariable(L"FAVIGNORE", FavoriteManager::getInstance()->hasIgnore(msgFrom)? L"1" : L"0");
        }
        ps.addVariable(L"NEW", Util::toStringW((int)newSession).c_str());
        ps.addValue(L"PRIVATE=1");
		}
        try {
                init.RecvMessage(ps.getParams(), message.c_str());
        } catch (Exception&) {
                externalFailure();
        }
}

void ChatBot::onMessageV2(const UserPtr& msgFrom, const tstring& message, bool newSession)
{
        try {
                init.RecvMessage2(newSession? BotInit::RECV_PM_NEW : BotInit::RECV_PM, Text::toT(msgFrom->getCID().toBase32()).c_str(), message.c_str(), (message.length()+1)*sizeof(WCHAR));
        } catch (Exception&) {
                externalFailure();
        }

}

ParamSet::ParamSet() : bufUsed(0)
{
        buf = (WCHAR*)malloc((bufSize = 4096) * sizeof(WCHAR));
}

ParamSet::~ParamSet()
{
        free(buf);
}

void ParamSet::addStr(const WCHAR* str)
{
        int sz = 0;
        for (int i = 0; str[i]; i++, sz++)
                if (str[i] == '|' || str[i] == '\\')
                        sz++;
        if (bufUsed + sz >= bufSize)
                buf = (WCHAR*)realloc(buf, (bufSize = bufSize*2 + sz+1) * sizeof(WCHAR));
        for (int j = 0; str[j]; j++) {
                if (str[j] == '|' || str[j] == '\\')
                        buf[bufUsed++] = '\\';
                buf[bufUsed++] = str[j];
        }
}

void ParamSet::putStr(const WCHAR* str, unsigned sz)
{
        if (bufUsed + sz >= bufSize)
                buf = (WCHAR*)realloc(buf, (bufSize = bufSize*2 + sz+1) * sizeof(WCHAR));
        if (sz == 1)
                buf[bufUsed] = *str;
        else
                memcpy(buf+bufUsed, str, sz * sizeof(WCHAR));
        bufUsed += sz;
}

void ParamSet::addVariable(const WCHAR *varName, const WCHAR *value)
{
        putStr(varName, wcslen(varName));
        putStr(L"=", 1);
        addStr(value);
        putStr(L"|", 1);
}

void ParamSet::addValue(const WCHAR *value)
{
        if (value) addStr(value);
        putStr(L"|", 1);
}

WCHAR* ParamSet::getParams()
{
        buf[bufUsed] = '\0';
        return buf;
}

WCHAR* ParamSet::cutParams()
{
        buf[bufUsed] = '\0';
        WCHAR *res = buf;
        buf = NULL;
        bufSize = bufUsed = 0;
        return res;
}
