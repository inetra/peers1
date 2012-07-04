//////////////////////////////////////////////////////////////////////////////////////
//
// Written by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
// For companies(Austin,TX): If you would like to get my resume, send an email.
//
// The source is free, but if you want to use it, mention my name and e-mail address
//
// History:
//    1.0      Initial version                  Zoltan Csizmadia
//
//////////////////////////////////////////////////////////////////////////////////////
//
// ExtendedTrace.cpp
//

// Include StdAfx.h, if you're using precompiled 
// header through StdAfx.h
#include "stdafx.h"

#if defined(_WIN32)

#define DBGHELP_TRANSLATE_TCHAR
#include <DbgHelp.h>
#include "ExtendedTrace.h"

#define BUFFERSIZE   0x200

static void checkBuggyLibrary(PCTSTR library) {
	std::vector<pair<tstring, tstring> > libraries;
	libraries.push_back(make_pair(L"Vlsp", L"V-One Smartpass"));
	libraries.push_back(make_pair(L"mclsp", L"McAfee AV"));
	libraries.push_back(make_pair(L"Niphk", L"Norman AV"));
	libraries.push_back(make_pair(L"aslsp", L"Aventail Corporation VPN"));
	libraries.push_back(make_pair(L"AXShlEx", L"Alcohol 120%"));
	libraries.push_back(make_pair(L"gdlsphlr", L"McAfee"));
	libraries.push_back(make_pair(L"mlang", L"IE"));
	libraries.push_back(make_pair(L"cslsp", L"McAfee"));
	libraries.push_back(make_pair(L"winsflt", L"PureSight Internet Content Filter"));
	libraries.push_back(make_pair(L"imslsp", L"ZoneLabs IM Secure"));
	libraries.push_back(make_pair(L"apitrap", L"Norton Cleansweep [?]"));
	libraries.push_back(make_pair(L"sockspy", L"BitDefender Antivirus"));
	libraries.push_back(make_pair(L"imon", L"Eset NOD32"));
	libraries.push_back(make_pair(L"KvWspXp(_1)", L"Kingsoft Antivirus"));
	libraries.push_back(make_pair(L"nl_lsp", L"NetLimiter"));
	libraries.push_back(make_pair(L"OSMIM", L"Marketscore Internet Accelerator"));
	libraries.push_back(make_pair(L"opls", L"Opinion Square [malware]"));
	libraries.push_back(make_pair(L"PavTrc", L"Panda Anti-Virus"));
	libraries.push_back(make_pair(L"pavlsp", L"Panda Anti-Virus"));
	libraries.push_back(make_pair(L"AppToPort", L"Wyvern Works  Firewall"));
	libraries.push_back(make_pair(L"SpyDll", L"Nice Spy [malware]"));
	libraries.push_back(make_pair(L"WBlind", L"Window Blinds"));
	libraries.push_back(make_pair(L"UPS10", L"Uniscribe Unicode Script Processor Library"));
	libraries.push_back(make_pair(L"SOCKS32", L"Sockscap [?]"));
	libraries.push_back(make_pair(L"___j", L"Worm: W32.Maslan.C@mm"));
	libraries.push_back(make_pair(L"nvappfilter", L"NVidia nForce Network Access Manager"));
	libraries.push_back(make_pair(L"mshp32", L"Worm: W32.Worm.Feebs"));
	libraries.push_back(make_pair(L"krn", L"NOD32 Antivirus")); //[+] FlylinkDC++
	libraries.push_back(make_pair(L"ProxyFilter", L"Hide My IP 2007"));
	libraries.push_back(make_pair(L"msui32", L"Malware MSUI32"));

	for(std::vector<pair<tstring, tstring> >::const_iterator i = libraries.begin(); i != libraries.end(); ++i) {
		tstring lib = i->first; 
		if(Util::stricmp(library, lib) == 0) {
			tstring app = i->second;
			size_t BUF_SIZE = TSTRING(LIB_CRASH).size() + app.size() + 16;
			AutoArray<TCHAR> buf(BUF_SIZE);
			snwprintf(buf, BUF_SIZE, CTSTRING(LIB_CRASH), app.c_str());
			MessageBox(0, buf, _T("Unhandled exception"), MB_OK);
			exit(1);
		}
	}
}

// Let's figure out the path for the symbol files
// Search path= ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;" + lpszIniPath
// Note: There is no size check for lpszSymbolPath!
static void InitSymbolPath(PTSTR lpszSymbolPath, PCTSTR lpszIniPath) {
	TCHAR lpszPath[BUFFERSIZE];

	// Creating the default path
	// ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	_tcscpy( lpszSymbolPath, _T("."));

	// environment variable _NT_SYMBOL_PATH
	if (GetEnvironmentVariable(_T("_NT_SYMBOL_PATH"), lpszPath, BUFFERSIZE)) {
		_tcscat(lpszSymbolPath, _T(";"));
		_tcscat(lpszSymbolPath, lpszPath);
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if (GetEnvironmentVariable(_T("_NT_ALTERNATE_SYMBOL_PATH"), lpszPath, BUFFERSIZE)) {
		_tcscat(lpszSymbolPath, _T(";"));
		_tcscat(lpszSymbolPath, lpszPath );
	}

	// environment variable SYSTEMROOT
	if (GetEnvironmentVariable(_T("SYSTEMROOT"), lpszPath, BUFFERSIZE)) {
		_tcscat(lpszSymbolPath, _T(";"));
		_tcscat(lpszSymbolPath, lpszPath );
		_tcscat(lpszSymbolPath, _T(";"));

		// SYSTEMROOT\System32
		_tcscat(lpszSymbolPath, lpszPath );
		_tcscat(lpszSymbolPath, _T("\\System32"));
	}

	// Add user defined path
	if (lpszIniPath != NULL && lpszIniPath[0] != '\0') {
		_tcscat(lpszSymbolPath, _T(";"));
		_tcscat(lpszSymbolPath, lpszIniPath);
	}
}

// Uninitialize the loaded symbol files
BOOL UninitSymInfo() {
	return SymCleanup(GetCurrentProcess());
}

// Initializes the symbol files
BOOL InitSymInfo(PCTSTR lpszInitialSymbolPath) {
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_LINES);
	TCHAR lpszSymbolPath[BUFFERSIZE];
	InitSymbolPath(lpszSymbolPath, lpszInitialSymbolPath);
	return SymInitialize(GetCurrentProcess(), lpszSymbolPath, TRUE);
}

// Get the module name from a given address
static BOOL GetModuleNameFromAddress(DWORD64 address, LPTSTR lpszModule) {
	IMAGEHLP_MODULE64   moduleInfo;
	memzero(&moduleInfo, sizeof(moduleInfo));
	moduleInfo.SizeOfStruct = sizeof(moduleInfo);
	if (SymGetModuleInfo64(GetCurrentProcess(), (DWORD)address, &moduleInfo)) {
		checkBuggyLibrary(moduleInfo.ModuleName);
		_tcscpy(lpszModule, moduleInfo.ModuleName);
		return TRUE;
	}
	else {
	   // Not found :(
		_tcscpy(lpszModule, _T("?"));
		return FALSE;
	}	
}

// Get function prototype and parameter info from ip address and stack address
static BOOL GetFunctionInfoFromAddresses(DWORD64 fnAddress, DWORD64 /*stackAddress*/, LPTSTR lpszSymbol) {
	BOOL              ret = FALSE;
	DWORD64           dwDisp = 0;
	const DWORD       dwSymSize = 1024*16;
	TCHAR             lpszUnDSymbol[BUFFERSIZE]=_T("?");
	PSYMBOL_INFO      pSym = (PSYMBOL_INFO)GlobalAlloc( GMEM_FIXED, dwSymSize );

	memzero(pSym, dwSymSize);
	pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSym->MaxNameLen = (dwSymSize - pSym->SizeOfStruct + 1) / sizeof(TCHAR);

   // Set the default to unknown
	_tcscpy( lpszSymbol, _T("?") );

	// Get symbol info for IP
	if (SymFromAddr(GetCurrentProcess(), fnAddress, &dwDisp, pSym))	{
	   // Make the symbol readable for humans
		UnDecorateSymbolName( pSym->Name, lpszUnDSymbol, BUFFERSIZE, 
			UNDNAME_COMPLETE | 
			UNDNAME_NO_THISTYPE |
			UNDNAME_NO_SPECIAL_SYMS |
			UNDNAME_NO_MEMBER_TYPE |
			UNDNAME_NO_MS_KEYWORDS |
			UNDNAME_NO_ACCESS_SPECIFIERS );

      // I am just smarter than the symbol file :)
		if ( _tcscmp(lpszUnDSymbol, _T("_WinMain@16")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("WinMain(HINSTANCE,HINSTANCE,LPCTSTR,int)"));
		else if ( _tcscmp(lpszUnDSymbol, _T("_main")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("main(int,TCHAR * *)"));
		else if ( _tcscmp(lpszUnDSymbol, _T("_mainCRTStartup")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("mainCRTStartup()"));
		else if ( _tcscmp(lpszUnDSymbol, _T("_wmain")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("wmain(int,TCHAR * *,TCHAR * *)"));
		else if ( _tcscmp(lpszUnDSymbol, _T("_wmainCRTStartup")) == 0 )
			_tcscpy(lpszUnDSymbol, _T("wmainCRTStartup()"));

		lpszSymbol[0] = _T('\0');
		LPCTSTR lpszParsed = lpszUnDSymbol;

#if 0
      // Let's go through the stack, and modify the function prototype, and insert the actual
      // parameter values from the stack
		if (_tcsstr(lpszUnDSymbol, _T("(void)")) == NULL && _tcsstr(lpszUnDSymbol, _T("()")) == NULL) {
			ULONG index = 0;
			for( ; ; index++ ) {
				LPTSTR lpszParamSep = _tcschr( const_cast<wchar_t*>(lpszParsed), _T(',') );
				if ( lpszParamSep == NULL )
					break;
				*lpszParamSep = _T('\0');
				_tcscat(lpszSymbol, lpszParsed);
				_stprintf(lpszSymbol + _tcslen(lpszSymbol), _T("=0x%08lX,"), *((ULONG*)(stackAddress) + 2 + index));
				lpszParsed = lpszParamSep + 1;
			}
			LPTSTR lpszParamSep = _tcschr( const_cast<wchar_t*>(lpszParsed), _T(')'));
			if (lpszParamSep != NULL) {
				*lpszParamSep = _T('\0');
				_tcscat( lpszSymbol, lpszParsed );
				_stprintf( lpszSymbol + _tcslen(lpszSymbol), _T("=0x%08lX)"), *((ULONG*)(stackAddress) + 2 + index) );
				lpszParsed = lpszParamSep + 1;
			}
		}
#endif
		_tcscat( lpszSymbol, lpszParsed );
		ret = TRUE;
	} 
#ifdef _DEBUG
	else {
		dcdebug("SymFromAddr error=%d\n", GetLastError());
	}
#endif
	GlobalFree(pSym);
	return ret;
}

// Get source file name and line number from IP address
// The output format is: "sourcefile(linenumber)" or
//                       "modulename!address" or
//                       "address"
static BOOL GetSourceInfoFromAddress(DWORD64 address, LPTSTR lpszSourceInfo) {
	IMAGEHLP_LINE64 lineInfo;
	DWORD          dwDisp;
	TCHAR          lpModuleInfo[BUFFERSIZE] = _T("");
	_tcscpy( lpszSourceInfo, _T("?(?)") );
	memzero(&lineInfo, sizeof(lineInfo));
	lineInfo.SizeOfStruct = sizeof(lineInfo);
	if (SymGetLineFromAddr64(GetCurrentProcess(), address, &dwDisp, &lineInfo)) {
		// Got it. Let's use "sourcefile(linenumber)" format
		_stprintf(lpszSourceInfo, _T("%s(%d)"), lineInfo.FileName, lineInfo.LineNumber);
		return TRUE;
	}
	else {
		// There is no source file information. :(
		// Let's use the "modulename!address" format
		GetModuleNameFromAddress( address, lpModuleInfo );

		if ( lpModuleInfo[0] == _T('?') || lpModuleInfo[0] == _T('\0'))
			// There is no modulename information. :((
			// Let's use the "address" format
			_stprintf( lpszSourceInfo, _T("0x%08X"), address );
		else
			_stprintf( lpszSourceInfo, _T("%s!0x%08X"), lpModuleInfo, address );
		return FALSE;
	}
}

tstring StackTrace(HANDLE hThread, const CONTEXT* inContext) {
	STACKFRAME64   callStack;
	CONTEXT        context = *inContext;
	TCHAR          symInfo[BUFFERSIZE] = _T("?");
	TCHAR          srcInfo[BUFFERSIZE] = _T("?");
	HANDLE         hProcess = GetCurrentProcess();

	// If it's not this thread, let's suspend it, and resume it at the end
	if (hThread != GetCurrentThread()) {
		if (SuspendThread(hThread) == -1) {
			return _T("No call stack\r\n");
		}
	}

	memzero(&callStack, sizeof(callStack));
	callStack.AddrPC.Offset    = inContext->Eip;
	callStack.AddrStack.Offset = inContext->Esp;
	callStack.AddrFrame.Offset = inContext->Ebp;
	callStack.AddrPC.Mode      = AddrModeFlat;
	callStack.AddrStack.Mode   = AddrModeFlat;
	callStack.AddrFrame.Mode   = AddrModeFlat;

	GetFunctionInfoFromAddresses(callStack.AddrPC.Offset, callStack.AddrFrame.Offset, symInfo);
	GetSourceInfoFromAddress(callStack.AddrPC.Offset, srcInfo);

	tstring f;
	f += srcInfo;
	f += _T(": ");
	f += symInfo;
	f += _T("\r\n");

	// Max 100 stack lines...
	for (ULONG index = 0; index < 100; index++) {
		BOOL bResult = StackWalk64(
			IMAGE_FILE_MACHINE_I386,
			hProcess,
			hThread,
			&callStack,
			&context, 
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL);

		if (index == 0)
			continue;

		if (!bResult || callStack.AddrFrame.Offset == 0) 
			break;

		GetFunctionInfoFromAddresses(callStack.AddrPC.Offset, callStack.AddrFrame.Offset, symInfo);
		GetSourceInfoFromAddress(callStack.AddrPC.Offset, srcInfo);

		f += srcInfo;
		f += _T(": ");
		f += symInfo;
		f += _T("\r\n");
	}
	if (hThread != GetCurrentThread()) {
		ResumeThread(hThread);
	}
	return f;
}

#endif //_DEBUG && _WIN32

/**
* @file
* $Id: ExtendedTrace.cpp,v 1.3 2008/05/03 11:17:30 alexey Exp $
*/
