/* 
* Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"
#include <atlctrlx.h>
#include "../client/PME.h"
#include "../client/HttpConnection.h"

#include "GeneralPage.h"
#include "../peers/PeersUtils.h"
#include "../peers/PeersVersion.h"
#include "../peers/InvokeLater.h"
#ifdef PPA_INCLUDE_IPFILTER
#include "../client/PGLoader.h"
#endif

PropPage::TextItem GeneralPage::texts[] = {
  { IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
  { IDC_SETTINGS_NICK, ResourceManager::NICK },
  { IDC_SETTINGS_EMAIL, ResourceManager::SETTINGS_GENERAL_EMAIL },
  { IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
#ifdef INCLUDE_PROVIDE_SELECTION
  { IDC_PROVIDER_GROUP, ResourceManager::SETTINGS_PROVIDER_TITLE },
#endif
  { IDC_SETTINGS_UPLOAD_SPEED, ResourceManager::SETTINGS_UPLOAD_LINE_SPEED },
  { IDC_SETTINGS_UPLOAD_SPEED2, ResourceManager::CONNECTION },
  { IDC_SETTINGS_MEBIBYES, ResourceManager::MBITSPS },
  { IDC_BW_SIMPLE, ResourceManager::SETTINGS_BWSINGLE },
  { IDC_BW_BOTH, ResourceManager::SETTINGS_BWBOTH },
  { IDC_SETTINGS_NOMINALBW, ResourceManager::SETTINGS_NOMINAL_BANDWIDTH },
  { IDC_SHOW_SPEED_CHECK, ResourceManager::SHOW_SPEED },
  { IDC_CHECK_SHOW_LIMITER, ResourceManager::SHOW_LIMIT },
  { IDC_CHECK_SHOW_SLOTS, ResourceManager::SHOW_SLOTS },
  { IDC_CD_GP, ResourceManager::CUST_DESC },
  { IDC_DU, ResourceManager::DU },
  { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item GeneralPage::items[] = {
  { IDC_NICK,		SettingsManager::NICK,			PropPage::T_STR }, 
  { IDC_NICK_ADD_UNIQUE_SUFFIX, SettingsManager::NICK_ADD_UNIQUE_SUFFIX, PropPage::T_BOOL },
  { IDC_EMAIL,		SettingsManager::EMAIL,			PropPage::T_STR }, 
  { IDC_DESCRIPTION,	SettingsManager::DESCRIPTION,	        PropPage::T_STR }, 
  { IDC_DOWN_COMBO,	SettingsManager::DOWN_SPEED,	        PropPage::T_STR },  
  { IDC_UP_COMBO,	SettingsManager::UP_SPEED,		PropPage::T_STR },  
  { IDC_SHOW_SPEED_CHECK, SettingsManager::SHOW_DESCRIPTION_SPEED, PropPage::T_BOOL },
  { IDC_CHECK_SHOW_LIMITER, SettingsManager::SHOW_DESCRIPTION_LIMIT, PropPage::T_BOOL },
  { IDC_CHECK_SHOW_SLOTS, SettingsManager::SHOW_DESCRIPTION_SLOTS, PropPage::T_BOOL },
  { 0, 0, PropPage::T_END }
};

bool RadioButtonList::isProviderSelected() {
  for (vector<CButton>::iterator i = providerButtons.begin(); i != providerButtons.end(); ++i) {
    if (i->GetCheck() == BST_CHECKED) {
      return true;
    }
  }
  return false;
}

void GeneralPage::validation() {
  tstring nick = Util::trim(GetDlgItemString(IDC_NICK));
  if (nick.empty()) {
    throw new PropPageValidationException(ResourceManager::SETTINGS_GENERAL_VALIDATION_NICK_EMPTY, IDC_NICK);
  }
#ifdef INCLUDE_PROVIDE_SELECTION
  if (!providerList.isProviderSelected()) {
    throw new PropPageValidationException(ResourceManager::SETTINGS_GENERAL_PROVIDER_NOT_SELECTED, 0);
  }
#endif
}

#ifdef INCLUDE_NANONET_PREFIX
class QueryNanoNetNumberThread : public PointerBase, HttpConnectionListener, InvokeLater::Listener {
private:
  InvokeLater* m_invoker;
  volatile bool m_done;
  const string m_nick;
  const string m_nickBase;
  HttpConnection connection;
  string m_buffer;
  virtual void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t *buf , size_t len) {
    m_buffer += string((const char*)buf, len);
  }
  virtual void on(Complete, HttpConnection*, const string&) throw() {
    dcdebug("QueryNanoNetNumberThread on Complete\n");
    m_buffer = Util::trim(m_buffer);
    PME pattern("^[\\d\\.]+$");
    dcassert(pattern.IsValid());
    if (pattern.match(m_buffer)) {
      if (SETTING(NICK) == m_nick) {
        SettingsManager::getInstance()->set(SettingsManager::NICK, "[" + m_buffer + "]" + m_nickBase);
      }
    }
    finalize();
  }
  virtual void on(Failed, HttpConnection*, const string&) throw() {
    dcdebug("QueryNanoNetNumberThread on Failed\n");
    finalize();
  }
  void finalize() {
    m_done = true;
    m_invoker->execute();
  }
  virtual bool executeLater() {
    dec();
    return true;
  }
public:
  QueryNanoNetNumberThread(const string& nick, const string& nickBase): m_done(false), connection(PeersUtils::getUserAgent()), m_nick(nick), m_nickBase(nickBase) {
    inc();
    // окно должно создаваться в главном потоке, где есть цикл обработки сообщений
    m_invoker = new InvokeLater(this);
    connection.addListener(this);
    connection.downloadFile(string(NANONET_NUMBER_SERVICE_URL) + m_nickBase);
  }

#ifdef _DEBUG
  ~QueryNanoNetNumberThread() {
    dcdebug("~QueryNanoNetNumberThread\n");
  }
#endif

  bool isDone() const {
    return m_done;
  }
};
#endif

int RadioButtonList::getSelectedProvider() {
  for (vector<CButton>::iterator i = providerButtons.begin(); i != providerButtons.end(); ++i) {
    if (i->GetCheck() == BST_CHECKED) {
      return i->GetWindowLong(GWL_USERDATA);
    }
  }
  return 0;
}

void GeneralPage::write()
{
#ifdef INCLUDE_NANONET_PREFIX
  const string initialNick = SETTING(NICK);
#endif
  PropPage::write(items);
#ifdef INCLUDE_PROVIDE_SELECTION
  const int newValue = providerList.getSelectedProvider();
  if (newValue > 0 && newValue != settings->get(SettingsManager::PROVIDER)) {
    settings->set(SettingsManager::PROVIDER, newValue);
#ifdef PPA_INCLUDE_IPFILTER
    PGLoader::getInstance()->LoadIPFilters();
#endif
  }
#endif
#ifdef INCLUDE_NANONET_PREFIX
  string newNick = SETTING(NICK);
  if (newNick != initialNick) {
    CWaitCursor wait;
    PME nanoNetPattern("^\\[[\\d\\.]+\\](.+)$");
    dcassert(nanoNetPattern.IsValid());
    string nickBase;
    if (nanoNetPattern.match(newNick)) {
      nickBase = nanoNetPattern[1];
    }
    else {
      nickBase = newNick;
    }
    dcassert(nickBase.length() <= newNick.length());
    Pointer<QueryNanoNetNumberThread> thread = new QueryNanoNetNumberThread(newNick, nickBase);
    for (int i = 0; !thread->isDone() && i < 10; ++i) {
      Sleep(100);
    }
  }
#endif

  // Save radio button
  int bw = SettingsManager::BWSETTINGS_DEFAULT;

  if(IsDlgButtonChecked(IDC_BW_BOTH))
    bw = SettingsManager::BWSETTINGS_ADVANCED;

  if (bw != SETTING(BWSETTING_MODE))
    settings->set(SettingsManager::BWSETTING_MODE, bw);

  TCHAR buf[1024];
  switch(bw) 
  {
  case SettingsManager::BWSETTINGS_DEFAULT: 
    GetDlgItemText(IDC_CONNECTION, buf, 1024);
    settings->set(SettingsManager::UPLOAD_SPEED, Text::fromT(tstring(buf)));
    break;
  case SettingsManager::BWSETTINGS_ADVANCED:
    ctrlConnectionType.GetLBText(ctrlConnectionType.GetCurSel(), buf);
    settings->set(SettingsManager::UPLOAD_SPEED, Text::fromT(tstring(buf)));
    break;
  }
}

LRESULT GeneralPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  PropPage::translate(texts);
  initHeader(IDC_SETTINGS_PERSONAL_INFORMATION);
#ifdef INCLUDE_PROVIDE_SELECTION
  initHeader(IDC_PROVIDER_GROUP);
  providerList.Create(GetDlgItem(IDC_PROVIDER_GROUP), NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | WS_TABSTOP);

  setCurrentSettingDelegate(SettingsManager::PROVIDER, MakeDelegate(this, &GeneralPage::getCurrentProvider));
  m_providerNotice.Create(GetDlgItem(IDC_PROVIDER_GROUP), NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  m_providerNotice.addWords(CTSTRING(SETTINGS_PROVIDER_NOTICE), m_providerNoticeFont);
  dcassert(providers.get() == NULL);
  providers.reset(new IpBlocksLoader());
  for (size_t i = 0; i < providers->size(); ++i) {
    CButton b;
    const DWORD styles = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_AUTORADIOBUTTON | WS_TABSTOP;
    b.Create(providerList, NULL, Text::toT((*providers)[i].name).c_str(), styles);
    b.SetWindowLong(GWL_USERDATA, (*providers)[i].id);
    if (SETTING(PROVIDER) == (*providers)[i].id) {
      b.SetCheck(BST_CHECKED);
    }
    b.SetFont(WinUtil::systemFont);
	providerList.addButton(b);
  }
  updateProviderNoticeLayout();
#else
  GetDlgItem(IDC_PROVIDER_GROUP).ShowWindow(SW_HIDE);
#endif
  ctrlConnection.Attach(GetDlgItem(IDC_CONNECTION));

  for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
    ctrlConnection.AddString(Text::toT(*i).c_str());

  PropPage::read(items);

  ctrlConnectionType.Attach(GetDlgItem(IDC_CONNECTIONTYPE));
  if (SETTING(USERLIST_IMAGE).empty()) {
    ConnTypes.CreateFromImage(IDB_USERS, 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
  } 
  else {
    ConnTypes.CreateFromImage(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE); 
  }
  ctrlConnectionType.SetImageList(ConnTypes);	

  ctrlDownloadSpeed.Attach(GetDlgItem(IDC_DOWN_COMBO));
  ctrlUploadSpeed.Attach(GetDlgItem(IDC_UP_COMBO));

  for(int i = 0; i < SettingsManager::SP_LAST; i++) {
    ctrlDownloadSpeed.AddString(Text::toT(SettingsManager::speeds[i]).c_str());
    ctrlUploadSpeed.AddString(Text::toT(SettingsManager::speeds[i]).c_str());
  }

  ctrlDownloadSpeed.SetCurSel(ctrlDownloadSpeed.FindString(0, Text::toT(SETTING(DOWN_SPEED)).c_str()));
  ctrlUploadSpeed.SetCurSel(ctrlUploadSpeed.FindString(0, Text::toT(SETTING(UP_SPEED)).c_str()));

  if(ctrlDownloadSpeed.GetCurSel() == -1 && !SETTING(DOWN_SPEED).empty()) {
    ctrlDownloadSpeed.AddString(Text::toT(SETTING(DOWN_SPEED)).c_str());
    ctrlDownloadSpeed.SetCurSel(ctrlDownloadSpeed.FindString(0, Text::toT(SETTING(DOWN_SPEED)).c_str()));
  }

  if(ctrlUploadSpeed.GetCurSel() == -1 && !SETTING(UP_SPEED).empty()) {
    ctrlUploadSpeed.AddString(Text::toT(SETTING(UP_SPEED)).c_str());
    ctrlUploadSpeed.SetCurSel(ctrlUploadSpeed.FindString(0, Text::toT(SETTING(UP_SPEED)).c_str()));
  }

  int q = 0;
  int pos = 0;
  bool compatibility = (SETTING(OLD_ICONS_MODE) != 0 && !SETTING(USERLIST_IMAGE).empty());
  for(size_t i = 0; i < 8; i++) {
    COMBOBOXEXITEM cbitem = {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
    tstring connType;
    switch(i) 
    {
    case 0: q = compatibility ? 1 : 6; connType = _T("Modem"); break;
    case 1: q = compatibility ? 2 : 6; connType = _T("ISDN"); break;
    case 2: q = compatibility ? 3 : 8; connType = _T("Satellite"); break;
    case 3: q = compatibility ? 4 : 8; connType = _T("Wireless"); break;
    case 4: q = compatibility ? 6 : 9; connType = _T("Cable"); break;
    case 5: q = compatibility ? 5 : 9; connType = _T("DSL"); break;
    case 6: q = compatibility ? 7 : 11; connType = _T("LAN(T1)"); break;
    case 7: q = compatibility ? 7 : 11; connType = _T("LAN(T3)"); break;
    }
    cbitem.pszText = const_cast<TCHAR*>(connType.c_str());
    cbitem.iItem = i; 
    cbitem.iImage = q;
    cbitem.iSelectedImage = q;
    ctrlConnectionType.InsertItem(&cbitem);

    if(connType == Text::toT(SETTING(UPLOAD_SPEED))) pos = i;

  }

  switch (SETTING(BWSETTING_MODE)) 
  {
  case SettingsManager::BWSETTINGS_ADVANCED: 
    CheckDlgButton(IDC_BW_BOTH, BST_CHECKED);
    ctrlConnectionType.SetCurSel(pos);
    break;
  default: 
    CheckDlgButton(IDC_BW_SIMPLE, BST_CHECKED);
    pos = ctrlConnection.FindString(0, Text::toT(SETTING(UPLOAD_SPEED)).c_str());
    ctrlConnection.SetCurSel(pos == CB_ERR ? 0 : pos);
    break;
  }

  fixControls();

  CEdit(GetDlgItem(IDC_NICK)).LimitText(35);
  CEdit(GetDlgItem(IDC_DESCRIPTION)).LimitText(50);
  CEdit(GetDlgItem(IDC_SETTINGS_EMAIL)).LimitText(64);
  return TRUE;
}

#ifdef INCLUDE_PROVIDE_SELECTION
int GeneralPage::getCurrentProvider(SettingsManager::IntSetting setting) {
#ifdef _DEBUG
  dcassert(setting == SettingsManager::PROVIDER);
#else
  UNREFERENCED_PARAMETER(setting);
#endif
  return providerList.getSelectedProvider();
}
#endif

LRESULT GeneralPage::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef INCLUDE_PROVIDE_SELECTION
  updateProviderNoticeLayout();
#endif
  return 0;
}

#ifdef INCLUDE_PROVIDE_SELECTION
void GeneralPage::updateProviderNoticeLayout() {
  const int radioButtonHeight = providerList.getButtonHeight();
  enum {
	  GROUP_TOP_PADDING = 32,
	  GROUP_BOTTOM_PADDING = 8,
	  GROUP_LEFT_PADDING = 8,
	  GROUP_RIGHT_PADDING = 8
  };
  enum {
	  GROUP_RIGHT_MARGIN = 8,
	  GROUP_BOTTOM_MARGIN = 8
  };
  CRect groupRect;
  CWindow group = GetDlgItem(IDC_PROVIDER_GROUP);
  group.GetClientRect(groupRect);
  group.MapWindowPoints(group.GetParent(), groupRect);
  CRect cRect;
  GetClientRect(cRect);
  groupRect.right = cRect.right - GROUP_RIGHT_MARGIN;
  groupRect.bottom = groupRect.top + GROUP_TOP_PADDING + GROUP_BOTTOM_PADDING + radioButtonHeight * providers->size();
  int bottomDelta;
  if (groupRect.bottom > cRect.bottom - GROUP_BOTTOM_MARGIN) {
	  groupRect.bottom = cRect.bottom - GROUP_BOTTOM_MARGIN;
	  bottomDelta = 0;
  }
  else {
	  bottomDelta = cRect.bottom - GROUP_BOTTOM_MARGIN - groupRect.bottom;
  }
  CRect listRect = groupRect;
  listRect.DeflateRect(GROUP_LEFT_PADDING, GROUP_TOP_PADDING, GROUP_RIGHT_PADDING, GROUP_BOTTOM_PADDING);
  group.GetParent().MapWindowPoints(group, listRect);
  m_providerNotice.updateLayout(listRect.left, listRect.top, listRect.Width());
  const int topDelta = m_providerNotice.getTextHeight() + radioButtonHeight / 2;
  groupRect.bottom += min(bottomDelta, topDelta);
  group.MoveWindow(groupRect);
  listRect.top += topDelta;
  listRect.bottom += min(bottomDelta, topDelta);
  providerList.MoveWindow(listRect);
  providerList.layoutButtons(radioButtonHeight, 0);
  providerList.setupScrollbar(radioButtonHeight);
}
#endif

LRESULT GeneralPage::onTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
  TCHAR buf[SETTINGS_BUF_LEN];
  GetDlgItemText(wID, buf, SETTINGS_BUF_LEN);
  const tstring old = buf;
  // Strip '$', '|', '<', '>' and ' ' from text
  TCHAR *b = buf, *f = buf, c;
  while ((c = *b++) != 0) {
    if (c != '$' && c != '|' && (wID == IDC_DESCRIPTION || c != ' ') && ( (wID != IDC_NICK && wID != IDC_DESCRIPTION && wID != IDC_SETTINGS_EMAIL) || (c != '<' && c != '>') ) )
      *f++ = c;
  }
  *f = '\0';
  if (old != buf) {
    // Something changed; update window text without changing cursor pos
    CEdit tmp(hWndCtl);
    int start, end;
    tmp.GetSel(start, end);
    tmp.SetWindowText(buf);
    if(start > 0) start--;
    if(end > 0) end--;
    tmp.SetSel(start, end);
    tmp.Detach();
  }
  return TRUE;
}

void GeneralPage::fixControls() {
  BOOL advanced = IsDlgButtonChecked(IDC_BW_BOTH) == BST_CHECKED;
  BOOL speed = IsDlgButtonChecked(IDC_SHOW_SPEED_CHECK) == BST_CHECKED;

  ::EnableWindow(GetDlgItem(IDC_DOWN_COMBO), speed);
  ::EnableWindow(GetDlgItem(IDC_UP_COMBO), speed);
  ::EnableWindow(GetDlgItem(IDC_SLASH), speed);
  ::EnableWindow(GetDlgItem(IDC_DU), speed);
  ::EnableWindow(GetDlgItem(IDC_SETTINGS_UPLOAD_SPEED2), advanced);
  ::EnableWindow(GetDlgItem(IDC_CONNECTIONTYPE), advanced);
  ::EnableWindow(GetDlgItem(IDC_SETTINGS_UPLOAD_SPEED), !advanced);
  ::EnableWindow(GetDlgItem(IDC_CONNECTION), !advanced);
  ::EnableWindow(GetDlgItem(IDC_SETTINGS_MEBIBYES), !advanced);

  if(ctrlConnection.GetCurSel() == -1) ctrlConnection.SetCurSel(0);
  if(ctrlConnectionType.GetCurSel() == -1) ctrlConnectionType.SetCurSel(0);

}

LRESULT GeneralPage::onClickedRadioButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  fixControls();
  return 0;
}

void RadioButtonList::GetScrollInfo(SCROLLINFO& si) {
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
	CWindow::GetScrollInfo(SB_VERT, &si);
}

void RadioButtonList::setScrollPos(int newPos) {
	SCROLLINFO si;
	si.fMask = SIF_POS;
	si.nPos = newPos;
	CWindow::SetScrollInfo(SB_VERT, &si);
	dcdebug("setScrollPos to %d\n", newPos);
	layoutButtons(getButtonHeight(), newPos);
}

LRESULT RadioButtonList::onVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SCROLLINFO si;
	GetScrollInfo(si);
	int newPos = si.nPos;
	switch (LOWORD(wParam))
	{
	case SB_TOP:			newPos = si.nMin; break;
	case SB_BOTTOM:			newPos = si.nMax - si.nPage + 1; break;
	case SB_LINEUP:			newPos -= getButtonHeight(); break;
	case SB_LINEDOWN:		newPos += getButtonHeight(); break;
	case SB_PAGEUP:			newPos -= max((int) si.nPage, getButtonHeight()); break;
	case SB_PAGEDOWN:		newPos += max((int) si.nPage, getButtonHeight()); break;
	case SB_THUMBPOSITION:	newPos = HIWORD(wParam); break;
	case SB_THUMBTRACK:		newPos = HIWORD(wParam); break;
	}
	newPos = adjustNewPos(newPos, si);	
	if (newPos != si.nPos) {
		setScrollPos(newPos);
	}
	return 0;
}

LRESULT RadioButtonList::onMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SCROLLINFO si;
	GetScrollInfo(si);
	int mouseDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
	UINT scrollLines = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
	int newPos = adjustNewPos(si.nPos - mouseDelta * scrollLines * getButtonHeight(), si);
	if (newPos != si.nPos) {
		setScrollPos(newPos);
	}
	return 0;
}

void RadioButtonList::layoutButtons(int radioButtonHeight, int offset) {
	CRect r;
	GetClientRect(r);
	r.top -= offset;
	for (vector<CButton>::iterator i = providerButtons.begin(); i != providerButtons.end(); ++i) {
		r.bottom = r.top + radioButtonHeight;
		i->MoveWindow(r);
		r.top += radioButtonHeight;
	}
}

void RadioButtonList::setupScrollbar(int radioButtonHeight) {
	CRect r;
	GetClientRect(r);
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	si.nMin = 0;
	si.nMax = radioButtonHeight * providerButtons.size() - 1;
	si.nPage = r.Height();
	si.nPos = 0;
	dcdebug("setupScrollbar %d,%d page %d\n", si.nMin, si.nMax, si.nPage);
	SetScrollInfo(SB_VERT, &si);
}

/**
* @file
* $Id: GeneralPage.cpp,v 1.15.2.4 2008/12/08 04:49:03 alexey Exp $
*/
