#include "stdinc.h"
#include "QoSManager.h"
#include "LogManager.h"

QoSManager::QoSManager(void)
{
	StringMap params;
    params["message"] = "Checking updates on startup";
	LOG(LogManager::SYSTEM, params);

#ifdef _WIN32
	mQOSCreateHandle = 0;
	mQOSCloseHandle = 0;
	mQOSAddSocketToFlow = 0;
	mQOSRemoveSocketFromFlow = 0;
	mQOSSetFlow = 0;

	// detect qWAVE available
	if (qWAVEhandle = LoadLibraryA("qwave.dll")) {
		// Windows >= Vista
		mQOSCreateHandle = (pfnQOSCreateHandle) GetProcAddress(qWAVEhandle, "QOSCreateHandle");
		mQOSCloseHandle = (pfnQOSCloseHandle) GetProcAddress(qWAVEhandle, "QOSCloseHandle");
		mQOSAddSocketToFlow = (pfnQOSAddSocketToFlow) GetProcAddress(qWAVEhandle, "QOSAddSocketToFlow");
		mQOSRemoveSocketFromFlow = (pfnQOSRemoveSocketFromFlow) GetProcAddress(qWAVEhandle, "QOSRemoveSocketFromFlow");
		mQOSSetFlow = (pfnQOSSetFlow) GetProcAddress(qWAVEhandle, "QOSSetFlow");

		if (!(mQOSCreateHandle && mQOSCloseHandle && mQOSAddSocketToFlow && mQOSRemoveSocketFromFlow && mQOSSetFlow)) {
			// bad qWAVE?
			params["message"] = "Fail GetProcAddress";
			LOG(LogManager::SYSTEM, params);
			FreeLibrary(qWAVEhandle);
			qWAVEhandle = NULL;
		}

		versionQoS.MajorVersion = 1;
		versionQoS.MinorVersion = 0;
		if (!mQOSCreateHandle(&versionQoS, &hQoS)) {
			params["message"] = "Fail QOSCreateHandle";
			LOG(LogManager::SYSTEM, params);
		} else {
			params["message"] = "QOSCreateHandle success!";
			LOG(LogManager::SYSTEM, params);
		}
		
		FlowId = 0;
	} else {
		params["message"] = "Fail loading qWAVE";
		LOG(LogManager::SYSTEM, params);
	}
#endif
}

QoSManager::~QoSManager(void)
{
#ifdef _WIN32
	if (qWAVEhandle != NULL) {
		mQOSCloseHandle(hQoS);
		FreeLibrary(qWAVEhandle);
	}
#endif
}

void QoSManager::markSocket(Socket &socket, PSOCKADDR destAddr, const char DSCPvalue) {
	StringMap params;
#ifdef _WIN32
	if (qWAVEhandle != NULL) {
		FlowId = 0;
		if (!mQOSAddSocketToFlow(hQoS, socket.sock, destAddr, QOSTrafficTypeBestEffort, QOS_NON_ADAPTIVE_FLOW, &FlowId)) {
			params["message"] = "QOSAddSocketToFlow return " + Util::toString(WSAGetLastError());
			LOG(LogManager::SYSTEM, params);
		} else {
			DWORD buf = DSCPvalue;
			if (!mQOSSetFlow(hQoS, FlowId, QOSSetOutgoingDSCPValue, sizeof(buf), &buf, 0, NULL)) {
				params["message"] = "QOSSetFlow return " + Util::toString(WSAGetLastError());
				LOG(LogManager::SYSTEM, params);
			} else {
				params["message"] = "QOSSetFlow success!";
				LOG(LogManager::SYSTEM, params);
			}
		}
	}
#endif

	// This works on Windows < Vista with 
	// DisableUserTOSSetting set to 0 in registry
	// see http://support.microsoft.com/kb/248611
	const char ipTOS = DSCPvalue << 2;
	setsockopt(socket.sock, IPPROTO_IP, IP_TOS, &ipTOS, sizeof(ipTOS));
}

void QoSManager::unmarkSocket(Socket &socket) {
// 
	(void)socket;
}
