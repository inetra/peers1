//-----------------------------------------------------------------------------
//(c) 2007 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------
#if !defined(FLYLINK_IPFILTER_PAGE_H)
#define FLYLINK_IPFILTER_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "../client/PGLoader.h"

class CFlylinkIPFilter :
  public PeersPropertyPage<IDD_FLYLINK_IPFILTERDIALOG>,
  public PropPageImpl<CFlylinkIPFilter,22>
{
  static TextItem texts[];

  BEGIN_MSG_MAP(CFlylinkIPFilter)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
  END_MSG_MAP()

  LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  string m_IPFilter;
  string m_IPFilterPATH;
  TCHAR *title;
public:
  CFlylinkIPFilter() {
    title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(TITLE_IPFILTER)).c_str());
    SetTitle(title);
  }
  virtual ~CFlylinkIPFilter(void) {
    free(title);
  }

  virtual void onActivate();
  virtual void write();
};

#endif
