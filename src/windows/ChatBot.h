
/*
 * ApexDC speedmod (c) SMT 2007
 */


#if !defined(CHAT_BOT_H)
#define CHAT_BOT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/User.h"
#include "../chatbot/ChatBotAPI.h"

// !SMT!-CB
class ChatBot : public Singleton<ChatBot> {
public:
        ChatBot();
        ~ChatBot();

        void onMessage(const UserPtr& msgFrom, const tstring& message, bool newSession);
        void onMessage(const string& huburl, const UserPtr& msgFrom, const string& message);
        void onUserAction(BotInit::CODES, const UserPtr& aUser);
        void onHubAction(BotInit::CODES, const string& hubUrl);
private:
        HINSTANCE hDll;
        BotInit init;
        int qrycount;

        void onMessageV1(const UserPtr& msgFrom, const tstring& message, bool newSession);
        void onMessageV2(const UserPtr& msgFrom, const tstring& message, bool newSession);
        void* botQueryInfo(int qryid, const WCHAR* objid, const void *param, unsigned paramsize);
        WCHAR* onQueryUserByCid(const WCHAR* cid);
        WCHAR* onQueryHubByUrl(const WCHAR* huburl);
        WCHAR* onQueryConnectedHubs();
        WCHAR* onQueryHubUsers(const WCHAR* huburl);
        WCHAR* onQueryRunningUploads(const WCHAR* cid);
        WCHAR* onQueryQueuedUploads(const WCHAR* cid);
        WCHAR* onQueryDownloads(const WCHAR* cid);
        void externalFailure();

        static void  __stdcall botSendMessage(const WCHAR *params, const WCHAR *message);
        static bool  __stdcall botSendMessage2(int msgid, const WCHAR* objid, const void *param, unsigned paramsize);
        static void* __stdcall botQueryInfo_rc(int qryid, const WCHAR* objid, const void *param, unsigned paramsize);
        static void  __stdcall botFreeInfo(void *info);
};


class ParamSet {
public:
        void addVariable(const WCHAR* varName, const WCHAR* value);
        void addValue(const WCHAR* value = NULL);
        WCHAR* getParams();
        WCHAR* cutParams();
        ParamSet();
        ~ParamSet();
private:
        void addStr(const WCHAR* str);
        void putStr(const WCHAR* str, unsigned sz);
        WCHAR *buf;
        unsigned bufSize, bufUsed;
};


#endif // CHAT_BOT_H
