/* POSSUM_MOD_BEGIN 

Most of this code was stolen/adapted/massacred from...

Module : FileTreeCtrl.h
Purpose: Interface for an MFC class which provides a tree control similiar 
         to the left hand side of explorer

Copyright (c) 1999 - 2003 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)
*/

#include <shlobj.h>
#include <lm.h>

//Class which gets stored int the item data on the tree control

class FolderTreeItemInfo
{
public:
	//Constructors / Destructors
	FolderTreeItemInfo();
	FolderTreeItemInfo(const FolderTreeItemInfo& ItemInfo);
	~FolderTreeItemInfo() {};

	//Member variables
	tstring			m_sFQPath;          //Fully qualified path for this item
	tstring			m_sRelativePath;    //The relative bit of the path
	NETRESOURCE*	m_pNetResource;     //Used if this item is under Network Neighborhood
	bool			m_bNetworkNode;     //Item is "Network Neighborhood" or is underneath it
};

//Class which encapsulates access to the System image list which contains
//all the icons used by the shell to represent the file system

class SystemImageList {
public:
  static CImageList getInstance();
protected:
  CImageList m_ImageList;
private:
  SystemImageList();
  ~SystemImageList() { }
};

//Struct taken from svrapi.h as we cannot mix Win9x and Win NT net headers in one program
#pragma pack(1)
struct FolderTree_share_info_50 
{
	char			shi50_netname[LM20_NNLEN+1];    /* share name */
	unsigned char 	shi50_type;						/* see below */
	unsigned short	shi50_flags;					/* see below */
	char FAR *		shi50_remark;                   /* ANSI comment string */
	char FAR *		shi50_path;                     /* shared resource */
	char			shi50_rw_password[SHPWLEN+1];   /* read-write password (share-level security) */
	char			shi50_ro_password[SHPWLEN+1];   /* read-only password (share-level security) */
};	/* share_info_50 */
#pragma pack()

//class which manages enumeration of shares. This is used for determining 
//if an item is shared or not
class ShareEnumerator
{
public:
	//Constructors / Destructors
	ShareEnumerator();
	~ShareEnumerator();

	//Methods
	void Refresh(); //Updates the internal enumeration list
	bool IsShared(const tstring& sPath);

protected:
	//Defines
	typedef NET_API_STATUS (WINAPI NT_NETSHAREENUM)(LPWSTR, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD, LPDWORD);
	typedef NET_API_STATUS (WINAPI NT_NETAPIBUFFERFREE)(LPVOID);
	typedef NET_API_STATUS (WINAPI WIN9X_NETSHAREENUM)(const char FAR *, short, char FAR *, unsigned short, unsigned short FAR *, unsigned short FAR *);

	//Data
	bool                     m_bWinNT;          //Are we running on NT
	HMODULE                  m_hNetApi;         //Handle to the net api dll
	NT_NETSHAREENUM*         m_pNTShareEnum;    //NT function pointer for NetShareEnum
	NT_NETAPIBUFFERFREE*     m_pNTBufferFree;   //NT function pointer for NetAPIBufferFree
	SHARE_INFO_502*          m_pNTShareInfo;    //NT share info
	WIN9X_NETSHAREENUM*      m_pWin9xShareEnum; //Win9x function pointer for NetShareEnum
	FolderTree_share_info_50* m_pWin9xShareInfo; //Win9x share info
	DWORD                    m_dwShares;        //The number of shares enumerated
};

//Allowable bit mask flags in SetDriveHideFlags / GetDriveHideFlags
const DWORD DRIVE_ATTRIBUTE_REMOVABLE   = 0x00000001;
const DWORD DRIVE_ATTRIBUTE_FIXED       = 0x00000002;
const DWORD DRIVE_ATTRIBUTE_REMOTE      = 0x00000004;
const DWORD DRIVE_ATTRIBUTE_CDROM       = 0x00000010;
const DWORD DRIVE_ATTRIBUTE_RAMDISK     = 0x00000020;

class FolderTree : public CWindowImpl<FolderTree, CTreeViewCtrl>
{
public:
	FolderTree();
        virtual ~FolderTree() { }
	
        BEGIN_MSG_MAP(FolderTree)
          MESSAGE_HANDLER(WM_DESTROY, onDestroy);
          REFLECTED_NOTIFY_CODE_HANDLER(NM_CLICK, OnClick);
          REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick);
          REFLECTED_NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnSelChanged);
          REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnItemExpanding);
          REFLECTED_NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnDeleteItem);
          DEFAULT_REFLECTION_HANDLER();
        END_MSG_MAP()

        LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
          // control has TVS_CHECKBOXES so state image list should be destroyed manually.
          GetImageList(TVSIL_STATE).Destroy();
          bHandled = FALSE;
          return 0;
        }

        LRESULT OnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled); // !SMT!-P
        LRESULT OnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled); // !SMT!-P

	LRESULT OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL &bHandled);
	LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	virtual LRESULT OnChecked(HTREEITEM hItem, BOOL &bHandled);
	virtual LRESULT OnUnChecked(HTREEITEM hItem, BOOL &bHandled);

	virtual void PopulateTree();
	virtual void Refresh();
	virtual tstring ItemToPath(HTREEITEM hItem) const;
	void Clear();
	HTREEITEM SetSelectedPath(const tstring& sPath, bool bExpanded = false);
	virtual bool IsDrive(HTREEITEM hItem);
	virtual bool IsDrive(const tstring& sPath);
	virtual bool IsFolder(const tstring& sPath);
	bool GetChecked(HTREEITEM hItem) const;
    BOOL SetChecked(HTREEITEM hItem, bool fCheck);
	void SetStaticCtrl(CStatic *staticCtrl);
	bool IsDirty();

protected:
	bool IsExpanded(HTREEITEM hItem);
	virtual int GetIconIndex(const tstring& sFilename);
	virtual int GetIconIndex(HTREEITEM hItem);
	virtual int GetIconIndex(LPITEMIDLIST lpPIDL);
	virtual int GetSelIconIndex(const tstring& sFilename);
	virtual int GetSelIconIndex(HTREEITEM hItem);
	virtual int GetSelIconIndex(LPITEMIDLIST lpPIDL);
	virtual HTREEITEM InsertFileItem(HTREEITEM hParent, FolderTreeItemInfo* pItem, bool bShared, int nIcon, int nSelIcon, bool bCheckForChildren);
	virtual void DisplayDrives(HTREEITEM hParent, bool bUseSetRedraw = true);
	virtual void DisplayPath(const tstring& sPath, HTREEITEM hParent, bool bUseSetRedraw = true);
	virtual tstring GetDriveLabel(const tstring& sDrive);
	tstring GetCorrectedLabel(FolderTreeItemInfo* pItem);
	virtual bool HasGotSubEntries(const tstring& sDirectory);
	virtual bool CanDisplayDrive(const tstring& sDrive);
	virtual bool IsShared(const tstring& sPath);
	static int CALLBACK CompareByFilenameNoCase(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void SetHasPlusButton(HTREEITEM hItem, bool bHavePlus);
	bool HasPlusButton(HTREEITEM hItem);
	void DoExpand(HTREEITEM hItem);
	HTREEITEM FindServersNode(HTREEITEM hFindFrom) const;
	virtual HTREEITEM FindSibling(HTREEITEM hParent, const tstring& sItem) const;
	virtual bool DriveHasRemovableMedia(const tstring& sPath);
	virtual bool IsMediaValid(const tstring& sDrive);
	virtual bool EnumNetwork(HTREEITEM hParent);
	virtual int DeleteChildren(HTREEITEM hItem, bool bUpdateChildIndicator);
	virtual BOOL GetSerialNumber(const tstring& sDrive, DWORD& dwSerialNumber);
	void SetHasSharedChildren(HTREEITEM hItem, bool bHasSharedChildren);
	void SetHasSharedChildren(HTREEITEM hItem);
	bool GetHasSharedChildren(HTREEITEM hItem);
	HTREEITEM HasSharedParent(HTREEITEM hItem);
	void ShareParentButNotSiblings(HTREEITEM hItem);
	void UpdateStaticCtrl();
	void UpdateChildItems(HTREEITEM hItem, bool bChecked);
	void UpdateParentItems(HTREEITEM hItem);
	
	//Member variables
	tstring			m_sRootFolder;
	HTREEITEM       m_hNetworkRoot;
	HTREEITEM       m_hMyComputerRoot;
	HTREEITEM       m_hRootedFolder;
	bool            m_bShowMyComputer;
	DWORD           m_dwDriveHideFlags;
	DWORD           m_dwFileHideFlags;
	COLORREF        m_rgbCompressed;
	bool            m_bShowCompressedUsingDifferentColor;
	COLORREF        m_rgbEncrypted;
	bool            m_bShowEncryptedUsingDifferentColor;
	bool            m_bDisplayNetwork;
	bool			m_bShowSharedUsingDifferentIcon;
	DWORD			m_dwMediaID[26];
	CComPtr<IMalloc>        m_pMalloc;
	CComPtr<IShellFolder>   m_pShellFolder;
	DWORD           m_dwNetworkItemTypes;
	bool            m_bShowDriveLabels;
	bool            m_bShowRootedFolder;
	CStatic*		m_pStaticCtrl;
	int64_t			m_nShareSizeDiff;
	bool			m_bDirty;

	ShareEnumerator theSharedEnumerator;
};

