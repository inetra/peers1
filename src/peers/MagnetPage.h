#pragma once

#include "../windows/PropPage.h"

class MagnetPage : 
	public PeersPropertyPage<IDD_MAGNETPAGE>,
	public PropPageImpl<MagnetPage,25>
{
private:
	tstring m_title;
public:
	MagnetPage() {
		m_title = TSTRING(SETTINGS_DOWNLOADS) + _T('\\') + TSTRING(SETTINGS_MAGNETPAGE);
		SetTitle(m_title.c_str());
	}
	virtual ~MagnetPage();

	virtual bool isExpertModePage() {
		return false;
	}

	BEGIN_MSG_MAP(MagnetPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	virtual void write();
private:
	static TextItem texts[];
	static Item items[];
};
