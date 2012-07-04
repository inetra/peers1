#include "../client/stdinc.h"
#include "../client/DCPlusPlus.h"
#include "SystemUtils.h"
#include "FileUtils.h"

string SystemUtils::describeError(int aError) {
	TCHAR msgBuf[2048];
	DWORD chars = FormatMessage( 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		aError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		msgBuf,
		COUNTOF(msgBuf),
		NULL 
		);
	if (chars > 0) {
		while (chars > 0 && (msgBuf[chars-1] == '\r' || msgBuf[chars-1] == '\n')) {
			--chars;
		}
		char resultBuf[sizeof(msgBuf)];
		int resultSize = WideCharToMultiByte(CP_ACP, 0, msgBuf, chars, resultBuf, COUNTOF(resultBuf), NULL, NULL);
		return string(resultBuf, resultSize);
	}
	else {
		return string();
	}
}

static tstring getFolder(int nFolder, LPCTSTR subfolder = NULL) {
	TCHAR buffer[MAX_PATH + 32];
	HRESULT hr = SHGetFolderPath(NULL, nFolder, NULL, SHGFP_TYPE_CURRENT, buffer);
	if (SUCCEEDED(hr)) {
		size_t len = _tcslen(buffer);
		if (len > 0 && buffer[len - 1] != _T('\\')) {
			buffer[len] = _T('\\');
			buffer[len + 1] = _T('\x0');
		}
		if (subfolder != NULL) {
			_tcscat(buffer, subfolder);
		}
		const tstring result = buffer;
		if (subfolder != NULL) {
			FileUtils::ensureDirectory(result);
		}
		return result;
	}
	return tstring();
}

tstring SystemUtils::getUserAppDataFolder() {
	return getFolder(CSIDL_APPDATA);
}

tstring SystemUtils::getAppDataFolder() {
#ifdef _DEBUG
	return getFolder(CSIDL_APPDATA, _T("Peers-Debug\\"));
#else
	return getFolder(CSIDL_APPDATA, _T("Peers\\"));
#endif
}

tstring SystemUtils::getProfileFolder() {
	return getFolder(CSIDL_PROFILE);
}

tstring SystemUtils::getDocumentFolder() {
	return getFolder(CSIDL_PERSONAL);
}
