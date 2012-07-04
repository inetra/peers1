#ifndef UserListColours_H
#define UserListColours_H

#include "PropPage.h"

class UserListColours : 
  public PeersPropertyPage<IDD_USERLIST_COLOURS>, 
  public PropPageImpl<UserListColours,6>
{
public:

	UserListColours() { 
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_USER_COLORS)).c_str());
		SetTitle(title);
	};

	~UserListColours() { free(title); };

	BEGIN_MSG_MAP(UserListColours)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_CHANGE_COLOR, BN_CLICKED, onChangeColour)
		COMMAND_HANDLER(IDC_IMAGEBROWSE, BN_CLICKED, onImageBrowse)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onChangeColour(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onImageBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	void write();

	CRichEditCtrl n_Preview;
private:
	void BrowseForPic(int DLGITEM);

	void refreshPreview();

	CListBox n_lsbList;
	int normalColour;
	int favoriteColour;
	int reservedSlotColour;
	int ignoredColour;
	int fastColour;
	int serverColour;
	int pasiveColour;
	int opColour; 
	int clientCheckedColour;
	int fileListCheckedColour;
	int fullCheckedColour;
	int badClientColour;
	int badFilelistColour;

protected:
	static Item items[];
	static TextItem texts[];
	TCHAR* title;
};

#endif //UserListColours_H

/**
 * @file
 * $Id: userlistcolours.h,v 1.5 2007/12/08 14:59:18 alexey Exp $
 */
