#if !defined __DEBUGMANAGER_H
#define __DEBUGMANAGER_H


#include "DCPlusPlus.h"
//[-]PPA [Doxygen 1.5.1] #include "Singleton.h"
#include "TimerManager.h"


class DebugManagerListener {
public:
template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> DebugCommand;
	typedef X<0> DebugDetection;	

	virtual void on(DebugCommand, const string&) throw() { }
	virtual void on(DebugDetection, const string&, int, const string&) throw() { }
};

class DebugManager : public Singleton<DebugManager>, public Speaker<DebugManagerListener>
{
	friend class Singleton<DebugManager>;
	DebugManager() { };
public:
	void SendCommandMessage(const string& mess, int typeDir, const string& ip) {
		fire(DebugManagerListener::DebugCommand(), mess, typeDir, ip);
	}
	void SendDetectionMessage(const string& mess) {
		fire(DebugManagerListener::DebugDetection(), mess);
	}
	~DebugManager() { };
	enum {
		HUB_IN, HUB_OUT, CLIENT_IN, CLIENT_OUT
	};
};
#define COMMAND_DEBUG(a,b,c) DebugManager::getInstance()->SendCommandMessage(a,b,c);
#define DETECTION_DEBUG(m) DebugManager::getInstance()->SendDetectionMessage(m);
#endif
