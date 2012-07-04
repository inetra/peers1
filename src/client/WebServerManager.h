/* 
 * Copyright (C) 2003 Twink, spm7@waikato.ac.nz
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
#if !defined(WEBSERVER_MANAGER_H)
#define WEBSERVER_MANAGER_H

#include "DCPlusPlus.h"
#include "ServerSocket.h"
#include "SettingsManager.h"
#include "SearchManager.h"
#include "Singleton.h"
#include "Thread.h"
#include "Speaker.h"
#include "CriticalSection.h"

#include <hash_set>

class WebServerListener{
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Setup;
	typedef X<1> ShutdownPC;

	virtual void on(Setup) throw() = 0;
	virtual void on(ShutdownPC, int action) throw() = 0;

};

class WebServerManager :  public Singleton<WebServerManager>, public ServerSocketListener, public Speaker<WebServerListener>, private SettingsManagerListener, private SearchManagerListener
{
public:
	ServerSocket& getServerSocket() {
		return socket;
	}

	string getPage(const string& file, const string& IP);
	string getLoginPage();


	// SettingsManagerListener
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
		if(BOOLSETTING(WEBSERVER)){
			Restart();
		} else {
			Stop();
		}
	}
	// SearchManagerListener
        void on(SearchManagerListener::SR, SearchResult* sr) throw() {
		onSearchResult(sr);
	}
		
	void onSearchResult(const SearchResult* aResult) throw();
	
	void Start();
	void Restart(){		
		Stop();
		Start();
	}

	string getResults() {
		return results;
	}

private:
#ifndef __INTEL_COMPILER
	friend Singleton<WebServerManager>;
#endif

	WebServerManager(void);
	~WebServerManager(void);

	
	void Stop(); 


	enum PageID{
		INDEX,
		DOWNLOAD_QUEUE,
		DOWNLOAD_FINISHED,
		UPLOAD_QUEUE,
		UPLOAD_FINISHED,
		SEARCH,
		LOG,
		SYSLOG,
		LOGOUT,
		PAGE_404
	};

	struct WebPageInfo {
		WebPageInfo(PageID n,string t) : id(n), title(t) {}
		PageID id;
		string title;
	};

	WebPageInfo *page404;

	struct eqstr{
		bool operator()(const char* s1, const char* s2) const {  
			return _stricmp(s1, s2) == 0;
		}
	};

	typedef hash_map <const char *, WebPageInfo*, hash<const char *>, eqstr> WebPages;
	WebPages pages;

	string getDLQueue();
	string getULQueue();
	string getFinished(bool);
	string getSearch();	
	string getLogs();
	string getSysLogs();
	string results;
	unsigned int results_size;
	bool sended_search;

	bool started;
	CriticalSection cs;
	// ServerSocketListener
	void on(ServerSocketListener::IncomingConnection) throw();

	ServerSocket socket;
	HWND m_hWnd;

	map<string, uint64_t> loggedin;
	int row;
public:
	void login(const string& ip){
		loggedin[ip] = GET_TICK();
	}
	void search(string search_str, int search_type) {
		if(sended_search == false) {
			int i = 0;
			while( (i = search_str.find("+", i)) != string::npos) {
				search_str.replace(i, 1, " ");
				i++;
			}
			if((SearchManager::TypeModes)search_type == SearchManager::TYPE_TTH) {
				search_str = "TTH:" + search_str;
			}
			SearchManager::getInstance()->addListener(this);
			SearchManager::getInstance()->search(WebServerManager::getInstance()->sClients, search_str, 0, (SearchManager::TypeModes)search_type, SearchManager::SIZE_DONTCARE, "manual");
			results = Util::emptyString;
			row = 0;
			sended_search = true;
		}
	}
	
	void reset() {
		row = 0; /* Counter to permit FireFox correct item clicks */
		SearchManager::getInstance()->removeListener(this);
	}

	bool isloggedin(const string& ip) {
		map<string, uint64_t>::iterator i;
		if((i = loggedin.find(ip)) != loggedin.end()) {
            uint64_t elapsed = (GET_TICK() - loggedin[ip]) / 1000;
			if(elapsed > 300) {
				loggedin.erase(i);
				return false;
			}

			return true;
		}

		return false;
	}

	StringList sClients;
};

class WebServerSocket : public Thread {
public:

	void accept(ServerSocket *s){
		int fromlen=sizeof(from);

		printf("accepting\n");
		sock = ::accept(s->getSock(), (struct sockaddr*)&from,&fromlen);
		u_long b = 1;
		ioctlsocket(sock, FIONBIO, &b);		
	}

	StringMap getArgs(const string& arguments); 
	int run();

private:
	sockaddr_in from;
	SOCKET sock;
	HANDLE thread;
};
#endif
