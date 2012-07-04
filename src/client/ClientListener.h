#ifndef CLIENTLISTENER_H_
#define CLIENTLISTENER_H_

#include "forward.h"

class ClientListener  
{
public:
	virtual ~ClientListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Connecting;
	typedef X<1> Connected;
	typedef X<2> UserIdentified;
	typedef X<3> UserUpdated;
	typedef X<4> UsersUpdated;
	typedef X<5> UserRemoved;
	typedef X<6> Redirect;
	typedef X<7> Failed;
	typedef X<8> GetPassword;
	typedef X<9> HubUpdated;
	typedef X<11> Message;
	//typedef X<12> StatusMessage;
	typedef X<13> PrivateMessage;
	typedef X<14> UserCommand;
	typedef X<15> HubFull;
	typedef X<16> NickTaken;
	typedef X<17> SearchFlood;
	typedef X<18> NmdcSearch;
	typedef X<19> AdcSearch;
	typedef X<20> CheatMessage;
	typedef X<21> HubTopic;
	typedef X<22> UserListReceived;

	virtual void on(Connecting, Client*) throw() { }
	virtual void on(Connected, Client*) throw() { }
	virtual void on(UserIdentified, Client*) throw() { }
	virtual void on(UserUpdated, Client*, const UserPtr&) throw() { } // !SMT!-fix
	virtual void on(UsersUpdated, Client*, const User::PtrList&) throw() { }
	virtual void on(UserRemoved, Client*, const UserPtr&) throw() { }
	virtual void on(Redirect, Client*, const string&) throw() { }
	virtual void on(Failed, Client*, const string&) throw() { }
	virtual void on(GetPassword, Client*) throw() { }
	virtual void on(HubUpdated, Client*) throw() { }
	virtual void on(Message, Client*, const UserPtr&, const string&) throw() { }
	//virtual void on(StatusMessage, Client*, const string&) throw() { }
	virtual void on(PrivateMessage, Client*, const UserPtr&, const UserPtr, const UserPtr, const string&, bool = true) throw() { } // !SMT!-S
	virtual void on(UserCommand, Client*, int, int, const string&, const string&) throw() { }
	virtual void on(HubFull, Client*) throw() { }
	virtual void on(NickTaken, Client*) throw() { }
	virtual void on(SearchFlood, Client*, const string&) throw() { }
	virtual void on(NmdcSearch, Client*, const string&, int, int64_t, int, const string&, bool) throw() { }
	virtual void on(AdcSearch, Client*, const AdcCommand&, const CID&) throw() { }
	virtual void on(CheatMessage, Client*, const string&) throw() { }
	virtual void on(HubTopic, Client*, const string&) throw() { }
	virtual void on(UserListReceived, Client*) throw() { }
};


#endif /*CLIENTLISTENER_H_*/
