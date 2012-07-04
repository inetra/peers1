#ifndef LimitPAGE_H
#define LimitPAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "../client/ConnectionManager.h"

class LimitPage : 
  public PeersPropertyPage<IDD_LIMITPAGE>, 
  public PropPageImpl<LimitPage,19>
{
public:
	LimitPage() {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_LIMIT)).c_str());
		SetTitle(title);
	};
	~LimitPage() {
		free(title);
	};

	BEGIN_MSG_MAP(LimitPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_TIME_LIMITING, onChangeCont)
		COMMAND_ID_HANDLER(IDC_THROTTLE_ENABLE, onChangeCont)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT onChangeCont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void write();

private:
	static Item items[];
	static TextItem texts[];
	CComboBox timeCtrlBegin, timeCtrlEnd;
	TCHAR* title;
	void fixControls();
};

#endif //LimitPAGE_H
