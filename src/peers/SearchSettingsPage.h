#pragma once
#include "../windows/PropPage.h"

class SearchSettingsPage : 
  public PeersPropertyPage<IDD_SEARCH_SETTINGS>, 
  public PropPageImpl<SearchSettingsPage,24>
{
private:
  tstring m_title;
public:
  SearchSettingsPage() {
    m_title = TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SEARCH);
    SetTitle(m_title.c_str());
  }

  virtual ~SearchSettingsPage() {
  }

  virtual void write();

  BEGIN_MSG_MAP(SearchSettingsPage)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
  END_MSG_MAP()

  LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:
  static ListItem listItems[];
  static TextItem texts[];

};
