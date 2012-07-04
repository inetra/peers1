#include "stdafx.h"
#include "AboutDlg.h"
#include "../peers/ControlAdjuster.h"

static const TCHAR thanks[] = 
_T("SMT - ApexDC++ s16.1\r\n")
_T("Paul - www ������\r\n")
_T("Iceberg - �����\r\n")
_T("M.S.A - ����, ������������ � ����\r\n")
_T("SkazochNik - ����, php-�������\r\n")
_T("KS!ON - ������� splash/icons/smiles\r\n")
_T("BugMaster - ����� ApexDC++ mod 2\r\n")
_T("Klirik - ����� StrongDC++ mod 5\r\n")
_T("necros - �����\r\n")
_T("Decker - �����\r\n")
_T("Drakon - �����\r\n")
_T("mt2006 - ���� ��������� wine(Linux)\r\n")
_T("WhiteD - �����\r\n")
_T("Squork - ����������\r\n")
_T("Berik - ���� �� �������\r\n")
_T("joedm - ���������� + ���������\r\n")
_T("Dodge-X - ������\r\n")
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
  m_Mail.m_tip.AddTool(m_Mail, _T("��������� �� �������"), &m_Mail.m_rcLink, 1);

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
