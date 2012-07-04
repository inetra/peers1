#include "stdafx.h"
#include "CFlylinkIPFilter.h"

PropPage::TextItem CFlylinkIPFilter::texts[] = {
  { IDC_FLYLINK_TRUST_IP_BOX, ResourceManager::IPFILTER_GROUPBOX },
  { IDC_FLYLINK_PATH_WAY, ResourceManager::IPFILTER_PATH_WAY },
  { IDC_FLYLINK_TP_IP, ResourceManager::IPFILTER_CUR_RULES },
  { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT CFlylinkIPFilter::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  PropPage::translate(texts);
  PropPage::initHeader(IDC_FLYLINK_TRUST_IP_BOX);
  try {
    m_IPFilterPATH = Util::getConfigPath() + "IPTrust.ini";
    string data = File(m_IPFilterPATH, File::READ, File::OPEN).read();
    m_IPFilter = data;
    SetDlgItemText(IDC_FLYLINK_TRUST_IP, Text::toT(data).c_str());
    GetDlgItem(IDC_FLYLINK_TRUST_IP).PostMessage(EM_SETSEL, 0, 0);
    GetDlgItem(IDC_FLYLINK_TRUST_IP).PostMessage(EM_SCROLLCARET);
    SetDlgItemText(IDC_FLYLINK_PATH, Text::toT(m_IPFilterPATH).c_str());
  } 
  catch (const FileException&) { 
    SetDlgItemText(IDC_FLYLINK_PATH, CTSTRING(ERR_IPFILTER)); 
  }
  return TRUE;
}

void CFlylinkIPFilter::write() {
#ifdef INCLUDE_PROVIDE_SELECTION
  const int provider = getCurrentSetting(SettingsManager::PROVIDER);
  if (provider == 0 || provider == PGLoader::UNLIM_ANY_PROVIDER) {
#endif
    string l_new = Text::fromT(GetDlgItemString(IDC_FLYLINK_TRUST_IP));
    if (l_new != m_IPFilter) {
      try {
        File fout(m_IPFilterPATH, File::WRITE, File::CREATE | File::TRUNCATE);
        fout.write(l_new);
        fout.close();
        PGLoader::getInstance()->LoadIPFilters();
      }
      catch (const FileException&) {
        return;
      }
    }
#ifdef INCLUDE_PROVIDE_SELECTION
  }
#endif
}

void CFlylinkIPFilter::onActivate() {
#ifdef INCLUDE_PROVIDE_SELECTION
  const int provider = getCurrentSetting(SettingsManager::PROVIDER);
#endif
  static const int controlIds[] = {
    IDC_FLYLINK_PATH,
    IDC_FLYLINK_PATH_WAY
  };
#ifdef INCLUDE_PROVIDE_SELECTION
  if (provider == 0 || provider == PGLoader::UNLIM_ANY_PROVIDER) {
#endif
    EnableWindows(controlIds, true);
    GetDlgItem(IDC_FLYLINK_TRUST_IP).EnableWindow(TRUE);
    SetDlgItemText(IDC_FLYLINK_TP_IP, CTSTRING(IPFILTER_CUR_RULES)); 
#ifdef INCLUDE_PROVIDE_SELECTION
  }
  else {
    EnableWindows(controlIds, false);
    GetDlgItem(IDC_FLYLINK_TRUST_IP).EnableWindow(FALSE);
    SetDlgItemText(IDC_FLYLINK_TP_IP, CTSTRING(IPFILTER_CONFIGURED_DEPENDING_ON_PROVIDER)); 
  }
#endif
}
