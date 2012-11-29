/* 
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdafx.h"

#ifdef _DEBUG_VLD   //[-] PPA VLD
/** 
	Memory leak detector
	You can remove following 3 lines if you don't want to detect memory leaks.
	Ignore STL pseudo-leaks, we can avoid them with _STLP_LEAKS_PEDANTIC, but it only slows down everything.
 */
#define VLD_MAX_DATA_DUMP 0
#define VLD_AGGREGATE_DUPLICATES
#include <vld.h>
#endif

#include "MainFrm.h"
#include "ExtendedTrace.h"
#include "ChatBot.h" // !SMT!-CB
#include "SplashWindow.h"
#include <delayimp.h>
#include <signal.h>
#ifdef PPA_INCLUDE_CHECK_UPDATE
#include "../peers/AutoUpdate.h"
#endif

#include "Application.h"
#include "ExceptionDlg.h"
#include "../peers/Shortcut.h"
#include "../peers/PeersUtils.h"
#include "../peers/Sounds.h"
#include "resource.h"

CAppModule _Module;

CriticalSection cs;
enum { DEBUG_BUFSIZE = 8192 };
static int recursion = 0;
// static char tth[192*8/(5*8)+2]; // !PPA!

#if 0
EXCEPTION_RECORD CurrExceptionRecord;
CONTEXT CurrContext;
#endif

#ifndef _DEBUG

FARPROC WINAPI FailHook(unsigned /* dliNotify */, PDelayLoadInfo  pdli) {
	char buf[DEBUG_BUFSIZE];
	sprintf(buf, APPNAME " just encountered and unhandled exception and will terminate.\nPlease do not report this as a bug. The error was caused by library %s.", pdli->szDll);
	MessageBox(WinUtil::mainWnd, Text::toT(buf).c_str(), _T(APPNAME) _T(" Has Crashed"), MB_OK | MB_ICONERROR);
	exit(-1);
}

#endif

#ifdef PPA_INCLUDE_SSL
#include "../client/SSLSocket.h"
#endif

const char* getExceptionName(DWORD code) {
	switch(code)
    { 
	case EXCEPTION_ACCESS_VIOLATION:		return "Access violation"; 
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:   return "Array out of range"; 
	case EXCEPTION_BREAKPOINT:				return "Breakpoint"; 
	case EXCEPTION_DATATYPE_MISALIGNMENT:   return "Read or write error"; 
	case EXCEPTION_FLT_DENORMAL_OPERAND:	return "Floating-point error"; 
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:      return "Floating-point division by zero"; 
	case EXCEPTION_FLT_INEXACT_RESULT:      return "Floating-point inexact result"; 
	case EXCEPTION_FLT_INVALID_OPERATION:   return "Unknown floating-point error"; 
	case EXCEPTION_FLT_OVERFLOW:			return "Floating-point overflow"; 
	case EXCEPTION_FLT_STACK_CHECK:         return "Floating-point operation caused stack overflow"; 
	case EXCEPTION_FLT_UNDERFLOW:			return "Floating-point underflow"; 
	case EXCEPTION_ILLEGAL_INSTRUCTION:     return "Illegal instruction"; 
	case EXCEPTION_IN_PAGE_ERROR:			return "Page error"; 
	case EXCEPTION_INT_DIVIDE_BY_ZERO:      return "Integer division by zero"; 
	case EXCEPTION_INT_OVERFLOW:			return "Integer overflow"; 
	case EXCEPTION_INVALID_DISPOSITION:     return "Invalid disposition"; 
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:return "Noncontinueable exception"; 
	case EXCEPTION_PRIV_INSTRUCTION:		return "Invalid instruction"; 
	case EXCEPTION_SINGLE_STEP:				return "Single step executed"; 
	case EXCEPTION_STACK_OVERFLOW:			return "Stack overflow"; 
	}
	return "";
}

LONG __stdcall DCUnhandledExceptionFilter( LPEXCEPTION_POINTERS e )
{	
#ifdef __DEBUG
	MessageBox(NULL, _T("If you would like to debug DCUnhandledExceptionFilter - attach to this process now"), _T(APPNAME), MB_OK);
#endif
	Lock l(cs);

	if(recursion++ > 30)
		exit(-1);

#ifndef _DEBUG
#if _MSC_VER == 1200
	__pfnDliFailureHook = FailHook;
#elif _MSC_VER == 1300 || _MSC_VER == 1310 || _MSC_VER == 1400
	__pfnDliFailureHook2 = FailHook;
#else
#error Unknown Compiler version
#endif

	// The release version loads the dll and pdb:s here...
	InitSymInfo(Text::toT(Util::getDataPath()).c_str());

#endif
	TCHAR pdbPath[MAX_PATH];
	GetModuleFileName(NULL, pdbPath, sizeof(pdbPath));
	TCHAR* dotPtr = _tcschr(pdbPath, '.');
	if (dotPtr != NULL) {
		_tcscpy(dotPtr, _T(".pdb"));
	}

	if (GetFileAttributes(pdbPath) == INVALID_FILE_ATTRIBUTES) {
		// No debug symbols, we're not interested...
		::MessageBox(WinUtil::mainWnd, _T(APPNAME) _T(" has crashed and you don't have .PDB file installed. Hence, I can't find out why it crashed, so don't report this as a bug unless you find a solution..."), _T(APPNAME) _T(" has crashed"), MB_OK);
#ifndef _DEBUG
		exit(1);
#else
		return EXCEPTION_CONTINUE_SEARCH;
#endif
	}
	File f(Util::getConfigPath() + "exceptioninfo.txt", File::WRITE, File::OPEN | File::CREATE);
	f.setEndPos(0);
	
	char buf[DEBUG_BUFSIZE];
	sprintf(buf, 
		"Code: %x (%s)\r\n"
		"Version: %d (%s)\r\n", 
		e->ExceptionRecord->ExceptionCode,
		getExceptionName(e->ExceptionRecord->ExceptionCode),
		BUILDID, 
		Util::getCompileDate().c_str());

	tstring exceptioninfo = Text::toT(buf);
	f.write(buf, strlen(buf));	
	
	OSVERSIONINFOEX ver;
	WinUtil::getVersionInfo(ver);
	const char *productType;
	if (ver.wProductType == VER_NT_DOMAIN_CONTROLLER) {
		productType = "domain controller";
	}
	else if (ver.wProductType == VER_NT_SERVER) {
		productType = "server";
	}
	else if (ver.wProductType == VER_NT_WORKSTATION) {
		productType = "workstation";
	}
	else {
		productType = "unknown product type";
	}

	sprintf(buf, "OS Version: %d.%d build %d service pack %d %s\r\n",
		(DWORD)ver.dwMajorVersion, (DWORD)ver.dwMinorVersion, (DWORD)ver.dwBuildNumber,
		(DWORD)ver.wServicePackMajor, productType);

	exceptioninfo += Text::toT(buf);
	f.write(buf, strlen(buf));

	time_t now;
	time(&now);
	strftime(buf, DEBUG_BUFSIZE, "Time: %Y-%m-%d %H:%M:%S\r\n", localtime(&now));

	exceptioninfo += Text::toT(buf);
	f.write(buf, strlen(buf));

#if 0
	WinUtil::exceptioninfo += LIT(_T("TTH: "));
	WinUtil::exceptioninfo += Text::toT(tth);
	WinUtil::exceptioninfo += LIT(_T("\r\n\r\n"));
#endif

    f.write(LIT("\r\n"));
	exceptioninfo += _T("\r\n");

        // !SMT!
        sprintf(buf,
	       "exception code %08X at eip=%08X, nested: %08X\r\n"
	       "eax=%08X ebx=%08X ecx=%08X edx=%08X\r\n"
          "esi=%08X edi=%08X ebp=%08X esp=%08X\r\n",
		e->ExceptionRecord->ExceptionCode, 
		e->ExceptionRecord->ExceptionAddress, 
		e->ExceptionRecord->ExceptionRecord,
		e->ContextRecord->Eax, 
		e->ContextRecord->Ebx,
		e->ContextRecord->Ecx, 
		e->ContextRecord->Edx,
		e->ContextRecord->Esi, 
		e->ContextRecord->Edi,
		e->ContextRecord->Ebp,
		e->ContextRecord->Esp);
	exceptioninfo += Text::toT(buf);
        f.write(buf, strlen(buf));

	const tstring trace = StackTrace(GetCurrentThread(), e->ContextRecord);

	f.write(LIT("\r\n"));
	f.write(Text::fromT(trace));
	f.close();

#if 0
	memcpy(&CurrExceptionRecord, e->ExceptionRecord, sizeof(EXCEPTION_RECORD));
	memcpy(&CurrContext, e->ContextRecord, sizeof(CONTEXT));
#endif

	exceptioninfo += _T("\r\n");
	exceptioninfo += trace;

	Sounds::PlaySound(SettingsManager::SOUND_EXC);

	if (WinUtil::mainWnd != NULL) {
		NOTIFYICONDATA m_nid = {0};
		m_nid.cbSize = sizeof(NOTIFYICONDATA);
		m_nid.hWnd = WinUtil::mainWnd;
		m_nid.uID = 0;
		m_nid.uFlags = NIF_INFO;
		m_nid.uTimeout = 5000;
		m_nid.dwInfoFlags = NIIF_WARNING;
		_tcsncpy(m_nid.szInfo, _T("exceptioninfo.txt was generated"), 255);
		_tcsncpy(m_nid.szInfoTitle, _T(APPNAME) _T(" has crashed"), 63);
		Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	}

	CExceptionDlg dlg(exceptioninfo);
	int iLastExceptionDlgResult = dlg.DoModal(WinUtil::mainWnd);
	if (iLastExceptionDlgResult == IDCANCEL) {
		ExitProcess(1);
	}

#ifndef _DEBUG
	UninitSymInfo();
	
	return EXCEPTION_CONTINUE_EXECUTION;
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif
}

static void sendCmdLine(HWND hOther, LPTSTR lpstrCmdLine)
{
	tstring cmdLine = lpstrCmdLine;
	LRESULT result;

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = sizeof(TCHAR)*(cmdLine.length() + 1);
	cpd.lpData = (void *)cmdLine.c_str();
	result = SendMessage(hOther, WM_COPYDATA, NULL,	(LPARAM)&cpd);
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	DWORD result;
	LRESULT ok = ::SendMessageTimeout(hWnd, WMU_WHERE_ARE_YOU, 0, 0,
		SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result);
	if(ok == 0)
		return TRUE;
	if(result == WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

static void checkCommonControls() {
#define PACKVERSION(major,minor) MAKELONG(minor,major)

	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	
	hinstDll = LoadLibrary(_T("comctl32.dll"));
	
	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
	
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
		
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			
			memzero(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			
			hr = (*pDllGetVersion)(&dvi);
			
			if(SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}
		
		FreeLibrary(hinstDll);
	}

	if(dwVersion < PACKVERSION(5,80)) {
		MessageBox(NULL, _T("Your version of windows common controls is too old for ") _T(APPNAME) _T(" to run correctly, and you will most probably experience problems with the user interface. You should download version 5.80 or higher from the DC++ homepage or from Microsoft directly."), _T("User Interface Warning"), MB_OK);
	}
}

static bool g_DisableSplash       = false;
bool g_RunningUnderWine    = false;

//[+]PPA
static bool running_under_wine()
{
    HMODULE module = GetModuleHandleA("ntdll.dll");
    if (!module)
		return 0;
    return (GetProcAddress(module, "wine_server_call") != NULL);
}

static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
 	checkCommonControls();

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);	
	
	SplashWindow splash(IDB_SPLASH);

        class PeersConfiguration : public StartupConfiguration, public FavoriteManagerInitializer {
        private:
          FavoriteHubEntry* findHub(FavoriteHubEntry::List& favoriteHubs, const string& server) const {
            if (!favoriteHubs.empty()) {
              for (FavoriteHubEntry::Iter i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
                FavoriteHubEntry* entry = *i;
                if (Util::stricmp(entry->getServer(), server) == 0) {
                  return entry;
                }
              }
              const string fullServerName = "dchub://" + server;
              for (FavoriteHubEntry::Iter i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
                FavoriteHubEntry* entry = *i;
                if (Util::stricmp(entry->getServer(), fullServerName) == 0) {
                  return entry;
                }
              }
            }
            return NULL;
          }

          void checkHub(FavoriteHubEntry::List& favoriteHubs, const string& server, const tstring& name, const tstring& description, bool forceAutoConnect, int mode) const {
            FavoriteHubEntry* e = findHub(favoriteHubs, server);
            if (e == NULL) {
              e = new FavoriteHubEntry();
              e->setName(Text::fromT(name));
              e->setConnect(true);
              e->setDescription(Text::fromT(description));
              e->setNick(Util::emptyString);
              e->setPassword(Util::emptyString);
              e->setServer(server);
              e->setUserDescription(Util::emptyString);
              e->setMode(mode);
              favoriteHubs.push_back(e);
            }
            else if (forceAutoConnect) {
#ifndef _DEBUG
              e->setConnect(true);
#endif
            }
          }
        public:

          virtual void onLoad(int& version, FavoriteHubEntry::List& favoriteHubs) const {
            checkHub(favoriteHubs, PeersUtils::PEERS_HUB, _T("Пирс"), _T("Файлообменная сеть Электронного города"), true, 0);
            // Always set mode auto for PEERS_HUB
            FavoriteHubEntry *peers = findHub(favoriteHubs, PeersUtils::PEERS_HUB);
            peers->setMode(0);
            checkHub(favoriteHubs, "p2p.academ.org", _T("Академ.Орг"), _T("Файлообменная сеть Академ-городка"), false, 2);
            if (version < 409) {
              FavoriteHubEntry* e = findHub(favoriteHubs, "dc.mcduck.info");
              if (e != NULL) {
                e->setConnect(false);
              }
            }
            if (version < BUILDID) {
              version = BUILDID;
            }
          }
        };
        PeersConfiguration configuration;
        configuration.m_clientId = APPNAME " V:" VERSIONSTRING;
        configuration.m_queueManagerVersion = VERSIONSTRING;
        configuration.m_settingsManagerVersion = Util::toString(BUILDID);
		configuration.m_mainHub = PeersUtils::PEERS_HUB;
	if (!g_DisableSplash) {
	  splash.init();
          Application::startup(splash.getCallback(), &configuration);
	}
	else {
          class NOPCallback : public ProgressCallback {
          public:
            virtual void showMessage(const tstring&) { }
          } nop;
	  Application::startup(&nop, &configuration);
	}
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();
	if (lst.empty()) {
		FavoriteManager::getInstance()->addPreviewApp("VLC", ".\\VLC\\VLCPortable.exe", "%[file]", "avi;divx;mpg;mpeg");
	}
	else if (lst.size() == 1 && (Util::stricmp(lst[0]->getApplication(),"AVIPreview.exe") == 0
		                      || Util::stricmp(lst[0]->getApplication(),".\\AVIPreview.exe") == 0)) {
		PreviewApplication fixed(*lst[0]);
		fixed.setName("VLC");
		fixed.setApplication(".\\VLC\\VLCPortable.exe");
		FavoriteManager::getInstance()->updatePreviewApp(0, fixed);
	}
	if (BOOLSETTING(DOWNLOAD_DIRECTORY_SHORTCUT)) {
		const string path = SETTING(DOWNLOAD_DIRECTORY);
		File::ensureDirectory(path);
		Shortcut::createDesktopShortcut(Text::toT(path), _T("Загрузки Peers"));
	}
/*
	if (Util::readRegistryBoolean(_T("SiteShortcut"), true)) {
		TCHAR path[MAX_PATH];
		GetModuleFileName(NULL, path, MAX_PATH);
		int iconIndex = 0;
		for (int i = 0; i < IDI_SITE_SHORTCUT; ++i) {
			if (LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(i)) != NULL) {
				++iconIndex;
			}
		}
		Shortcut::createInternetShortcut(_T("http://www.cn.ru/"), _T("Портал cn.ru"), path, iconIndex);
	}
*/
        ChatBot::newInstance(); // !SMT!-CB

	if(ResourceManager::getInstance()->isRTL()) {
		SetProcessDefaultLayout(LAYOUT_RTL);
	}

        int nRet; {// !SMT!-fix this will ensure that GUI (wndMain) destroyed before client library shutdown (gui objects may call lib)

	MainFrame wndMain;

	CRect rc = wndMain.rcDefault;

	if( (SETTING(MAIN_WINDOW_POS_X) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_POS_Y) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_SIZE_X) != CW_USEDEFAULT) &&
		(SETTING(MAIN_WINDOW_SIZE_Y) != CW_USEDEFAULT) ) {

		rc.left = SETTING(MAIN_WINDOW_POS_X);
		rc.top = SETTING(MAIN_WINDOW_POS_Y);
		rc.right = rc.left + SETTING(MAIN_WINDOW_SIZE_X);
		rc.bottom = rc.top + SETTING(MAIN_WINDOW_SIZE_Y);
		// Now, let's ensure we have sane values here...
		if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
			rc = wndMain.rcDefault;
		}
	}

	int rtl = ResourceManager::getInstance()->isRTL() ? WS_EX_RTLREADING : 0;
	if(wndMain.CreateEx(NULL, rc, 0, rtl | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) == NULL) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

// Backup & Archive Settings at Starup!!! Written by Drakon.
	if (BOOLSETTING(STARTUP_BACKUP))
	{
		tstring bkcmd = Text::toT(Util::getDataPath()) + _T("BackUp/BackupProfile.bat");
		ShellExecute(NULL, NULL, bkcmd.c_str(), NULL, NULL, SW_HIDE);
	}
// End of BackUp...

        DestroyAndDetachWindow(splash);
	if(nCmdShow == SW_SHOWMINIMIZED || BOOLSETTING(MINIMIZE_ON_STARTUP)) {
		wndMain.ShowWindow(SW_SHOWMINIMIZED);
	} else {
		wndMain.ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
	}
        nRet = theLoop.Run();

        ChatBot::deleteInstance(); // !SMT!-CB
	_Module.RemoveMessageLoop();
        } // !SMT!-fix
 	Application::shutdown();
	return nRet;
}

void __cdecl AbortSignalHandler(int)
{
    __asm int 3
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
  g_RunningUnderWine = running_under_wine();
#ifdef PPA_INCLUDE_CHECK_UPDATE
  if (_tcsstr(lpstrCmdLine, _T(PEERS_OPTION_NO_UPDATE)) == NULL) {
		string updatePath = Text::fromT(AutoUpdate::getUpdateTargetPath());
	  //TODO нужно еще exists(wstring), чтобы строку не преобразовывать
		if (File::exists(updatePath) && File::getSize(updatePath) > 0) {
		  if (AutoUpdate::execute()) {
			  return AutoUpdate::EXIT_TO_UPDATE;
		  }
	  }
  }
  //TODO else вывести сообщение если архив обновления еще есть.
#endif
#ifndef _DEBUG
	SingleInstance dcapp(_T("{DOMODC-AEE8350A-B49A-4753-AB4B-E55479A48351}"));
#else
	SingleInstance dcapp(_T("{DOMODC-AEE8350A-B49A-4753-AB4B-E55479A48350}"));
#endif
        // !SMT!-UI  displaying abort message will disable generating exceptioninfo.txt
        signal(SIGABRT, AbortSignalHandler);
#ifndef SMT_CUSTOM
        _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif

	bool multipleInstances = false;
	bool magnet = false;
	bool delay = false;

#ifdef _DEBUG
	g_DisableSplash = true;
#endif
	if(_tcsstr(lpstrCmdLine, _T("/nologo"))!=NULL || _tcsstr(lpstrCmdLine, _T("/startup"))!=NULL) {
		g_DisableSplash = true;
		nCmdShow = SW_SHOWMINIMIZED;
	}
	if(_tcsstr(lpstrCmdLine, _T("/wine"))!=NULL)
		g_RunningUnderWine = true;
	if(_tcsstr(lpstrCmdLine, _T("/q"))!=NULL)
		multipleInstances = true;
	if(_tcsstr(lpstrCmdLine, _T("/magnet"))!=NULL)
		magnet = true;
	if(_tcsstr(lpstrCmdLine, _T("/c")) != NULL) {
		multipleInstances = true;
		delay = true;
	}

	if(dcapp.IsAnotherInstanceRunning()) {
		// Allow for more than one instance...
		bool multiple = false;
		if (multipleInstances == false && magnet == false) {
			if (::MessageBox(NULL, _T(APPNAME) _T(" уже запущена, или еще не закончила операцию закрытия...\nВы уверены, что хотите загрузить еще одну копию программы?"), 
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
				multiple = true;
			}
		} else {
			multiple = true;
		}

		if(delay == true) {
			Thread::sleep(2500);		// let's put this one out for a break
		}

		if(multiple == false || magnet == true) {
			HWND hOther = NULL;
			EnumWindows(searchOtherInstance, (LPARAM)&hOther);

			if (hOther != NULL) {
				// pop up
				::SetForegroundWindow(hOther);
				//if (IsIconic(hOther)) {
                                        // ::ShowWindow(hOther, SW_RESTORE); // !SMT!-f - disable, it unlocks password-protected instance
				//}
				sendCmdLine(hOther, lpstrCmdLine);
			}
			return FALSE;
		}
	}

	srand((unsigned) time(NULL));
	// For SHBrowseForFolder, UPnP
	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
	// before next function call
	Util::initialize();
#ifdef _DEBUG
	InitSymInfo(Text::toT(Util::getDataPath()).c_str());
#endif
	LPTOP_LEVEL_EXCEPTION_FILTER pOldSEHFilter = NULL;
	pOldSEHFilter = SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
	
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
        /* !PPA!
        try {
                File f(WinUtil::getAppName(), File::READ, File::OPEN);
                TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));
                size_t n = 0;
                size_t n2 = DEBUG_BUFSIZE;
                while( (n = f.read(buf, n2)) > 0) {
                        tth.update(buf, n);
                        n2 = DEBUG_BUFSIZE;
                }
                tth.finalize();
                strcpy(::tth, tth.getRoot().toBase32().c_str());
                WinUtil::tth = Text::toT(::tth);
        } catch(const FileException&) {
                dcdebug("Failed reading exe\n");
        }
        */

	HINSTANCE hInstRich = ::LoadLibrary(_T("RICHED20.DLL"));	

	int nRet = Run(lpstrCmdLine, nCmdShow);
 
	if ( hInstRich ) {
		::FreeLibrary(hInstRich);
	}
	
	// Return back old VS SEH handler
	if (pOldSEHFilter != NULL) {
		SetUnhandledExceptionFilter(pOldSEHFilter);
	}

	_Module.Term();
	::CoUninitialize();
	::WSACleanup();
#ifdef _DEBUG
	UninitSymInfo();
#endif
	return nRet;
}

/**
 * @file
 * $Id: main.cpp,v 1.21.2.6 2008/12/08 19:26:51 alexey Exp $
 */
