#if !defined(QOS_MANAGER_H)
#define QOS_MANAGER_H

#include "Singleton.h"
#include "Socket.h"

#ifdef _WIN32

#include <windows.h>
#include "peers\qos2.h"

#endif

class QoSManager: public Singleton<QoSManager>
{
	friend class Singleton<QoSManager>;
public:
	void markSocket(Socket &socket, PSOCKADDR destAddr, const char DSCPvalue);
	void unmarkSocket(Socket &socket);
private:
	QoSManager(void);
	~QoSManager(void);

#ifdef _WIN32
	HMODULE qWAVEhandle;
	HANDLE hQoS;
	QOS_VERSION versionQoS;


	// QOS2 external functions
	typedef BOOL (WINAPI *pfnQOSCreateHandle)(PQOS_VERSION Version, PHANDLE QOSHandle);
	typedef BOOL (WINAPI *pfnQOSCloseHandle)(HANDLE QOSHandle);
	typedef BOOL (WINAPI *pfnQOSAddSocketToFlow)(HANDLE QOSHandle, SOCKET Socket, PSOCKADDR DestAddr, QOS_TRAFFIC_TYPE TrafficType, DWORD Flags, PQOS_FLOWID FlowId);
	typedef BOOL (WINAPI *pfnQOSRemoveSocketFromFlow)(HANDLE QOSHandle, SOCKET Socket, QOS_FLOWID FlowId, DWORD Flags);
	typedef BOOL (WINAPI *pfnQOSSetFlow)(HANDLE QOSHandle, QOS_FLOWID FlowId, QOS_SET_FLOW Operation, ULONG Size, PVOID Buffer, DWORD Flags, LPOVERLAPPED Overlapped);

	pfnQOSCreateHandle mQOSCreateHandle;
	pfnQOSCloseHandle mQOSCloseHandle;
	pfnQOSAddSocketToFlow mQOSAddSocketToFlow;
	pfnQOSRemoveSocketFromFlow mQOSRemoveSocketFromFlow;
	pfnQOSSetFlow mQOSSetFlow;

	QOS_FLOWID FlowId;
#endif
};

#endif //QOS_MANAGER_H