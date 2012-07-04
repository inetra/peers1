/*
*
* Based on a class by R. Engels
* http://www.codeproject.com/shell/shellcontextmenu.asp
*/

#if !defined(SHELLCONTEXTMENU_H)
#define SHELLCONTEXTMENU_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CShellContextMenu
{
public:
	CShellContextMenu();
	~CShellContextMenu();

	void SetPath(const tstring& strPath);
	CMenu* GetMenu();
	UINT ShowContextMenu(HWND hWnd, CPoint pt);

private:
	bool bDelete;
	CMenu* m_Menu;
	IShellFolder* m_psfFolder;
	LPITEMIDLIST* m_pidlArray;

	void FreePIDLArray(LPITEMIDLIST* pidlArray);
	//void SHBindToParentEx(LPCITEMIDLIST pidl, REFIID riid, LPVOID* ppv, LPCITEMIDLIST* ppidlLast);
	bool GetContextMenu(LPVOID* ppContextMenu, int& iMenuType);
	void InvokeCommand(LPCONTEXTMENU pContextMenu, UINT idCommand);
	static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif // !defined(SHELLCONTEXTMENU_H)
