#include "stdafx.h"
#include "AboutDlg.h"
#include "../peers/ControlAdjuster.h"

static const TCHAR thanks[] = 
_T("SMT - ApexDC++ s16.1\r\n")
_T("Paul - www сервер\r\n")
_T("Iceberg - Форум\r\n")
_T("M.S.A - сайт, документация и идеи\r\n")
_T("SkazochNik - сайт, php-скрипты\r\n")
_T("KS!ON - графика splash/icons/smiles\r\n")
_T("BugMaster - патчи ApexDC++ mod 2\r\n")
_T("Klirik - патчи StrongDC++ mod 5\r\n")
_T("necros - патчи\r\n")
_T("Decker - патчи\r\n")
_T("Drakon - патчи\r\n")
_T("mt2006 - патч поддержки wine(Linux)\r\n")
_T("WhiteD - патчи\r\n")
_T("Squork - багрепорты\r\n")
_T("Berik - дока по модемам\r\n")
_T("joedm - багрепорты + албанский\r\n")
_T("Dodge-X - баннер\r\n")
;

LRESULT AboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  SetDlgItemText(IDC_VERSION, _T(APPNAME) _T(VERSIONSTRING));
  initVersionTip();
  CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
  ctrlThanks.FmtLines(TRUE);
  ctrlThanks.AppendText(thanks, TRUE);
  ctrlThanks.Detach();
  //[-]PPA		SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
  //SetDlgItemText(IDC_LATEST, CTSTRING(DOWNLOADING));
  SetDlgItemText(IDC_TOTALS, (_T("Upload: ") + Util::formatBytesW(SETTING(TOTAL_UPLOAD)) + _T(", Download: ") + 
    Util::formatBytesW(SETTING(TOTAL_DOWNLOAD))).c_str());
  SetDlgItemText(IDC_LINK, HOMEPAGE);
  SetDlgItemText(IDC_COMPT, Text::toT("Compiled on: " + Util::getCompileDate()).c_str());

  url.SubclassWindow(GetDlgItem(IDC_LINK));
  url.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON|HLINK_UNDERLINEHOVER);
  url.m_tip.AddTool(url, HOMEPAGE, &url.m_rcLink, 1);

  m_Mail.SubclassWindow(GetDlgItem(IDC_SUPPORT_MAIL));
  m_Mail.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON|HLINK_UNDERLINEHOVER);
  m_Mail.m_tip.AddTool(m_Mail, _T("Сообщайте об ошибках"), &m_Mail.m_rcLink, 1);

  if(SETTING(TOTAL_DOWNLOAD) > 0) {
    TCHAR buf[256];
    snwprintf(buf, sizeof(buf), _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));

    SetDlgItemText(IDC_RATIO, buf);
    /*	snwprintf(buf, sizeof(buf), "Uptime: %s", Util::formatTime(Util::getUptime()));
    SetDlgItemText(IDC_UPTIME, Text::toT(buf).c_str());*/
  }
  CenterWindow(GetParent());
  return TRUE;
}

void AboutDlg::initVersionTip() {
  m_versionTip.Create(m_hWnd, rcDefault, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX);
  tstring versionHint(Text::toT(VERSIONFILE));
  const int lastSlash = versionHint.find_last_of(_T('/'));
  if (lastSlash != tstring::npos) {
    versionHint = versionHint.substr(0, lastSlash);
  }
  CRect versionRect;
  GetDlgItem(IDC_VERSION).GetClientRect(versionRect);
  GetDlgItem(IDC_VERSION).MapWindowPoints(m_hWnd, versionRect);
  CToolInfo ti(TTF_SUBCLASS, m_hWnd, 1, versionRect, const_cast<LPTSTR>(versionHint.c_str()));
  ControlAdjuster::fixToolInfoSize(ti);
  m_versionTip.AddTool(ti);
}

LRESULT AboutDlg::onMailLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  WinUtil::openLink(Text::toT("mailto:team@flylinkdc.ru").c_str());
  return 0;
}

LRESULT AboutDlg::onLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  WinUtil::openLink(HOMEPAGE);
  return 0;
}
