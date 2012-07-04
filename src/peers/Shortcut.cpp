#include "stdafx.h"
#include <intshcut.h>
#include "Shortcut.h"

void Shortcut::createDesktopShortcut(const tstring& fileName, const tstring& displayName) {
	CComPtr<IMalloc> malloc;
	SHGetMalloc(&malloc);
	CComPtr<IShellFolder> folder;
	CComPtr<IEnumIDList> folderEnum;
	if (SHGetDesktopFolder(&folder)==NOERROR && folder->EnumObjects(0,SHCONTF_NONFOLDERS,&folderEnum) == NOERROR) {
		CComPtr<IShellLink> shellLink;
		shellLink.CoCreateInstance(CLSID_ShellLink);
		LPITEMIDLIST p;
		ULONG enumCount;
		while (folderEnum->Next(1,&p,&enumCount)==NOERROR && enumCount > 0) {
			TCHAR c[MAX_PATH];
			if (SHGetPathFromIDList(p, c)) {
				CComQIPtr<IPersistFile> file = shellLink;
				file->Load(c, STGM_READ);
			}
			WIN32_FIND_DATA wfd;
			shellLink->GetPath(c,sizeof(c),&wfd,SLGP_UNCPRIORITY);
			if (Util::stricmp(c, fileName.c_str()) == 0) return;
		}
		malloc->Free(p);
	}
	CComPtr<IShellLink> shellLink;
	shellLink.CoCreateInstance(CLSID_ShellLink);
	shellLink->SetPath(fileName.c_str());
	LPITEMIDLIST p;
	TCHAR c[MAX_PATH];
	if (SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&p) == NOERROR && SHGetPathFromIDList(p,c)) {
		CComQIPtr<IPersistFile> file = shellLink;
		file->Save((tstring(c) + _T(PATH_SEPARATOR_STR) + displayName + _T(".lnk")).c_str(), FALSE);
		malloc->Free(p);
	}
}

void Shortcut::createInternetShortcut(const tstring& url, const tstring& displayName, const tstring& iconFilename, int iconIndex) {
	{
		LPITEMIDLIST p;
		TCHAR c[MAX_PATH];
		if (SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&p) == NOERROR && SHGetPathFromIDList(p,c)) {
			tstring path = tstring(c) + _T(PATH_SEPARATOR_STR) + displayName + _T(".url");
			if (File::exists(Text::fromT(path))) {
				return;
			}
			CIniFile file;
			file.SetFilename(path.c_str());
			file.PutString(_T("InternetShortcut"), _T("URL"), url.c_str()) ;
			file.PutString(_T("InternetShortcut"), _T("IconIndex"), Util::toStringW(iconIndex).c_str()) ;
			file.PutString(_T("InternetShortcut"), _T("IconFile"), iconFilename.c_str());
			SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, path.c_str(), NULL);
			return;
		}
	}
#if 0
	CComPtr<IMalloc> malloc;
	SHGetMalloc(&malloc);
	CComPtr<IShellFolder> folder;
	CComPtr<IEnumIDList> folderEnum;
	if (SHGetDesktopFolder(&folder)==NOERROR && folder->EnumObjects(0,SHCONTF_NONFOLDERS,&folderEnum) == NOERROR) {
		CComQIPtr<IUniformResourceLocator, &IID_IUniformResourceLocator> link;
		if (FAILED(CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (void **)&link))) {
			return;
		}
		LPITEMIDLIST p;
		ULONG enumCount;
		while (folderEnum->Next(1,&p,&enumCount)==NOERROR && enumCount > 0) {
			TCHAR c[MAX_PATH];
			if (SHGetPathFromIDList(p, c)) {
				CComQIPtr<IPersistFile> file = link;
				file->Load(c, STGM_READ);
			}
			LPTSTR linkUrl;
			HRESULT hr = link->GetURL(&linkUrl);
			if (hr == S_OK) {
				// TODO check why GetURL() does not work.
				if (Util::stricmp(linkUrl, url.c_str()) == 0) {
					malloc->Free(p);
					return;
				}
				malloc->Free(p);
			}
		}
		malloc->Free(p);
	}
	CComQIPtr<IUniformResourceLocator, &IID_IUniformResourceLocator> link;
	if (FAILED(CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (void **)&link))) {
		return;
	}
	link->SetURL(url.c_str(), IURL_SETURL_FL_GUESS_PROTOCOL);
	if (!iconFilename.empty()) {
		// IID_IPropertySetStorage
		CComPtr<IPropertySetStorage> psps;
		link.QueryInterface(&psps);
		// Open ProertySet
		CComPtr<IPropertyStorage> pps;
		psps->Open(FMTID_Intshcut, STGM_READWRITE, &pps );
		// Array of Propertyps to write (by PROP_ID)
		PROPSPEC ppids[2] = { {PRSPEC_PROPID, PID_IS_ICONINDEX}, {PRSPEC_PROPID, PID_IS_ICONFILE} };
		// Array of Property-Variants
		PROPVARIANT ppvar[2];
		// Initialize PropVars
		PropVariantInit( ppvar );
		PropVariantInit( ppvar + 1 );
		ppvar[0].vt = VT_I4; // Index is I4
		ppvar[0].lVal = iconIndex; // Get the icon
		ppvar[1].vt = VT_LPWSTR; // Iconfile is LPWSTR
		TCHAR buffer[MAX_PATH];
		_tcscpy(buffer, iconFilename.c_str());
		ppvar[1].pwszVal = buffer; // Name of Iconfile
		// Write Propertydata
		pps->WriteMultiple(2, ppids, ppvar, 0);
		// Commit Propertydata (flush)
		pps->Commit( STGC_DEFAULT );
	}
	LPITEMIDLIST p;
	TCHAR c[MAX_PATH];
	if (SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&p) == NOERROR && SHGetPathFromIDList(p,c)) {
		CComQIPtr<IPersistFile> file = link;
		file->Save((tstring(c) + _T(PATH_SEPARATOR_STR) + displayName + _T(".url")).c_str(), FALSE);
		malloc->Free(p);
	}
#endif
}
