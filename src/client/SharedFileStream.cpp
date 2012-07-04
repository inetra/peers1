/* 
 * Copyright (C) 2003-2005 RevConnect, http://www.revconnect.com
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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "SharedFileStream.h"

#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union
#include "Winioctl.h"

CriticalSection SharedFileStream::critical_section;
SharedFileStream::SharedFileHandleMap SharedFileStream::file_handle_pool;

typedef BOOL (__stdcall *SetFileValidDataFunc) (HANDLE, LONGLONG); 
static SetFileValidDataFunc setFileValidData = NULL;


SharedFileHandle::SharedFileHandle(const string& name, bool shareDelete)
{
	DWORD dwShareMode = FILE_SHARE_READ;
	DWORD dwDesiredAccess = GENERIC_READ;

	if(shareDelete){
		dwShareMode |= (FILE_SHARE_DELETE | FILE_SHARE_WRITE);
	}else{
		dwDesiredAccess |= GENERIC_WRITE;
	}

	handle = ::CreateFile(Text::utf8ToWide(name).c_str(), 
					dwDesiredAccess, 
					dwShareMode, 
					NULL, 
					OPEN_ALWAYS, 
					FILE_FLAG_SEQUENTIAL_SCAN,
					NULL);
	
	if(handle == INVALID_HANDLE_VALUE) {
		throw FileException(Util::translateError(GetLastError()));
	}

	if(!SETTING(ANTI_FRAG)){
		DWORD bytesReturned;
		DeviceIoControl(handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytesReturned, NULL);
	}
}

SharedFileStream::SharedFileStream(const string& name, int64_t _pos, int64_t size, bool shareDelete) 
    : pos(_pos)
{
	{
		Lock l(critical_section);

		if(file_handle_pool.count(name) > 0){

	        shared_handle_ptr = file_handle_pool[name];
			shared_handle_ptr->ref_cnt++;

			return;

		}else{

	        shared_handle_ptr = new SharedFileHandle(name, shareDelete);
	        shared_handle_ptr->ref_cnt = 1;
			file_handle_pool[name] = shared_handle_ptr;

		}
	}

	if((size > 0) && BOOLSETTING(ANTI_FRAG)){
		Lock l(shared_handle_ptr->m_cs);

		DWORD x = (DWORD)(size >> 32);
		::SetFilePointer(shared_handle_ptr->handle, (DWORD)(size & 0xffffffff), (PLONG)&x, FILE_BEGIN);
		::SetEndOfFile(shared_handle_ptr->handle);

		if(setFileValidData != NULL){
			if(!setFileValidData(shared_handle_ptr->handle, size))
				dcdebug("SetFileValidData error %d\n", GetLastError());
		}
    }

}

SharedFileStream::~SharedFileStream()
{
	Lock l(critical_section);

	shared_handle_ptr->ref_cnt--;
	
	if(!shared_handle_ptr->ref_cnt)
	{
        for(SharedFileHandleMap::iterator i = file_handle_pool.begin();
        							i != file_handle_pool.end();
                                    i++)
		{
			if(i->second == shared_handle_ptr)
			{
            	file_handle_pool.erase(i);
				delete shared_handle_ptr;
                return;
            }
        }

		_ASSERT(0);
    }
}

size_t SharedFileStream::write(const void* buf, size_t len) throw(Exception)
{
	Lock l(shared_handle_ptr->m_cs);

	DWORD x = (DWORD)(pos >> 32);
	DWORD ret = ::SetFilePointer(shared_handle_ptr->handle, (DWORD)(pos & 0xffffffff), (PLONG)&x, FILE_BEGIN);

	if(ret == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		throw FileException(Util::translateError(GetLastError()));

	if(!::WriteFile(shared_handle_ptr->handle, buf, len, &x, NULL)) {
		throw FileException(Util::translateError(GetLastError()));
	}

    pos += len;
	return len;
}

size_t SharedFileStream::read(void* buf, size_t& len) throw(Exception) {
	Lock l(shared_handle_ptr->m_cs);

	DWORD x = (DWORD)(pos >> 32);
	::SetFilePointer(shared_handle_ptr->handle, (DWORD)(pos & 0xffffffff), (PLONG)&x, FILE_BEGIN);

	if(!::ReadFile(shared_handle_ptr->handle, buf, len, &x, NULL)) {
		throw(FileException(Util::translateError(GetLastError())));
	}
	len = x;

    pos += len;

	return x;
}


// For SetFileValidData() - antifrag purpose
static struct EnsurePrivilege
{
	EnsurePrivilege(){
		HANDLE			 hToken;
		TOKEN_PRIVILEGES privilege;
		LUID			 luid;
		DWORD			 dwRet;
		OSVERSIONINFO    osVersionInfo;

		typedef BOOL (__stdcall *OpenProcessTokenFunc) (HANDLE, DWORD, PHANDLE);
		typedef BOOL (__stdcall *LookupPrivilegeValueFunc) (LPCTSTR, LPCTSTR, PLUID);
		typedef BOOL (__stdcall *AdjustTokenPrivilegesFunc) (HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

		OpenProcessTokenFunc openProcessToken = NULL;
		LookupPrivilegeValueFunc lookupPrivilegeValue = NULL;
		AdjustTokenPrivilegesFunc adjustTokenPrivileges = NULL;

		osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
		dwRet = GetVersionEx(&osVersionInfo);

		if(!dwRet){
			dcdebug("GetVersionEx error : %d\n", GetLastError());
			return;
		}

		if(osVersionInfo.dwMajorVersion < 5) return;
		if(osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion < 1) return;

		HMODULE hModule;

		hModule = GetModuleHandle(_T("advapi32"));
		if(hModule == NULL) return;

		openProcessToken = (OpenProcessTokenFunc)GetProcAddress(hModule, "OpenProcessToken");

	#ifdef  UNICODE
		lookupPrivilegeValue = (LookupPrivilegeValueFunc)GetProcAddress(hModule, "LookupPrivilegeValueW");
	#else
		lookupPrivilegeValue = (LookupPrivilegeValueFunc)GetProcAddress(hModule, "LookupPrivilegeValueA");
	#endif
		adjustTokenPrivileges = (AdjustTokenPrivilegesFunc)GetProcAddress(hModule, "AdjustTokenPrivileges");

		if(openProcessToken == NULL || lookupPrivilegeValue == NULL || adjustTokenPrivileges == NULL)
			return;

		dcdebug("Os version %d.%d\n", osVersionInfo.dwMajorVersion, osVersionInfo.dwMinorVersion);

		dwRet = openProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);

		if(!dwRet){
			dcdebug("OpenProcessToken error : %d\n", GetLastError());
			return;
		}

		dwRet = lookupPrivilegeValue(NULL, SE_MANAGE_VOLUME_NAME, &luid);
		if(!dwRet){
			dcdebug("LookupPrivilegeValue error : %d\n", GetLastError());
			goto cleanup;
		}

		privilege.PrivilegeCount = 1;
		privilege.Privileges[0].Luid = luid;
		privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		dwRet = adjustTokenPrivileges(hToken, FALSE, &privilege, 0, NULL, NULL);

		if(!dwRet){
			dcdebug("AdjustTokenPrivileges error : %d\n", GetLastError());
			goto cleanup;
		}

		hModule = GetModuleHandle(_T("kernel32"));
		if(hModule)
			setFileValidData = (SetFileValidDataFunc)GetProcAddress(hModule, "SetFileValidData");

		dcdebug("ensurePrivilege done.\n");
	cleanup:
		CloseHandle(hToken);
	}
} X;
