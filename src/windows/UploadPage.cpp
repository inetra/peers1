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

#include "UploadPage.h"
#include "HashProgressDlg.h"
#include "LineDlg.h"

#include "../client/ShareManager.h"
#include "../peers/PeersVersion.h"
#include "../peers/CaptionFont.h"
#include "../peers/ControlAdjuster.h"
#include "../peers/ShareManagerRefreshParticipant.h"

static tstring prepareSmallSlotNotice() {
  tstring text = TSTRING(SETCZDC_NOTE_SMALL_UP);
  int pos = text.find(_T("%s"));
  if (pos != tstring::npos) {
	  text.replace(pos, 2, Util::formatBytesW(1024 * SETTING(SET_MINISLOT_SIZE), true));
  }
  return text;
}

PropPage::TextItem UploadPage::texts[] = {
  { IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_UPLOADS },
  { IDC_UPLOADS_SUBTITLE, ResourceManager::SETTINGS_UPLOADS_SUBTITLE },
  { IDC_SETTINGS_SHARE_SIZE, ResourceManager::SETTINGS_SHARE_SIZE, PropPage::AUTOSIZE_HORIZ_ALWAYS }, 
  { IDC_SETTINGS_UPLOADS_SLOTS, ResourceManager::SETTINGS_UPLOADS_SLOTS },
  { IDC_CZDC_SMALL_SLOTS, ResourceManager::SETCZDC_SMALL_UP_SLOTS },
  { IDC_CZDC_NOTE_SMALL, ResourceManager::SETCZDC_NOTE_SMALL_UP, PropPage::AUTOSIZE_VERT, &prepareSmallSlotNotice },
  { IDC_SLOT_DL, ResourceManager::EXTRASLOT_TO_DL }, // !SMT!-S
  { IDC_SETTINGS_AUTO_REFRESH_TIME, ResourceManager::SETTINGS_AUTO_REFRESH_TIME, PropPage::AUTOSIZE_HORIZ_ALWAYS },
  { IDC_SETTINGS_AUTO_REFRESH_TIMEUNIT, ResourceManager::SETTINGS_AUTO_REFRESH_TIMEUNIT, PropPage::AUTOSIZE_HORIZ_ALWAYS },
  { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item UploadPage::items[] = {
  { IDC_SLOTS, SettingsManager::SLOTS, PropPage::T_INT }, 
  { IDC_EXTRA_SLOTS, SettingsManager::EXTRA_SLOTS, PropPage::T_INT },
  { IDC_SLOT_DL, SettingsManager::EXTRASLOT_TO_DL, PropPage::T_BOOL }, // !SMT!-S
  { IDC_AUTO_REFRESH_TIME, SettingsManager::AUTO_REFRESH_TIME, PropPage::T_INT },
  { 0, 0, PropPage::T_END }
};

void UploadPage::initFonts() {
  {  
    CaptionFont font(GetFont(), 7, 8);
    smallFont.Attach(font.Detach());
    GetDlgItem(IDC_CZDC_NOTE_SMALL).SetFont(smallFont);
  }
  {
    CaptionFont font(GetFont(), CaptionFont::BOLD);
    totalFont.Attach(font.Detach());
    ctrlTotal.SetFont(totalFont);
  }
}

LRESULT UploadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	addPostSaveParticipant(ShareManagerRefreshParticipant::NAME, ShareManagerRefreshParticipant::execute);
  ctrlTotal.Attach(GetDlgItem(IDC_TOTAL));
  initFonts();
  PropPage::translate(texts);
  initHeader(IDC_SETTINGS_SHARED_DIRECTORIES);
  helpLink.SetHyperLink((tstring(HELP_REDIRECT_URL) + _T("upload")).c_str());
  helpLink.Create(m_hWnd, NULL, CTSTRING(SETTINGS_UPLOAD_HELP_LINK));
  static const int totalRowControlsIds[] = { IDC_SETTINGS_SHARE_SIZE, IDC_TOTAL };
  layoutControls(totalRowControlsIds);
  static const int autoRefreshControlsIds[] = { IDC_SETTINGS_AUTO_REFRESH_TIME, IDC_AUTO_REFRESH_TIME, IDC_SETTINGS_AUTO_REFRESH_TIMEUNIT };
  layoutControls(autoRefreshControlsIds);
  CUpDownCtrl(GetDlgItem(IDC_REFRESH_SPIN)).SetBuddy(GetDlgItem(IDC_AUTO_REFRESH_TIME));
  
  PropPage::read(items);
  ctrlTotal.SetWindowText(Util::formatBytesW(ShareManager::getInstance()->getShareSize()).c_str());

  CUpDownCtrl(GetDlgItem(IDC_REFRESH_SPIN)).SetRange(60, 60*24);
  CUpDownCtrl(GetDlgItem(IDC_SLOTSPIN)).SetRange(1, 500);
  CUpDownCtrl(GetDlgItem(IDC_EXTRA_SLOTS_SPIN)).SetRange(3, 100);

  ft.SubclassWindow(GetDlgItem(IDC_TREE1));
  ft.SetStaticCtrl(&ctrlTotal);
  ft.PopulateTree();
  return TRUE;
}

LRESULT UploadPage::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (helpLink) {
    CRect r;
    GetClientRect(r);
    int helpW, helpH;
    helpLink.GetIdealSize(helpW, helpH, r.Width() / 2);
    CRect headerRect;
    GetDlgItem(IDC_SETTINGS_SHARED_DIRECTORIES).GetWindowRect(headerRect);
    ScreenToClient(headerRect);
    helpLink.MoveWindow(r.right - helpW, max(0, (int) (headerRect.top + headerRect.bottom - helpH) / 2), helpW, helpH);
  }
  return 0;
}

void UploadPage::onExpertModeToggle() {
  static const int totalRowControlsIds[] = { 
    IDC_SETTINGS_AUTO_REFRESH_TIME, 
    IDC_AUTO_REFRESH_TIME, 
    IDC_REFRESH_SPIN,
    IDC_SETTINGS_AUTO_REFRESH_TIMEUNIT,
    IDC_SETTINGS_UPLOADS_SLOTS,
    IDC_SLOTS,
    IDC_SLOTSPIN,
    IDC_CZDC_SMALL_SLOTS,
    IDC_EXTRA_SLOTS,
    IDC_EXTRA_SLOTS_SPIN,
    IDC_CZDC_NOTE_SMALL,
    IDC_SLOT_DL
  };
  ShowWindows(totalRowControlsIds, isExpertMode());
}

LRESULT UploadPage::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if ((HWND) lParam == (HWND) ctrlTotal) {
    CDCHandle dc((HDC) wParam);
    dc.SetTextColor(GREEN_COLOR);
    dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
    return (LRESULT) GetSysColorBrush(COLOR_BTNFACE);
  }
  else {
    bHandled = FALSE;
    return 0;
  }
}

LRESULT UploadPage::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
  const HDROP drop = (HDROP) wParam;
  const UINT nrFiles = DragQueryFile(drop, (UINT) -1, NULL, 0);
  for (UINT i = 0; i < nrFiles; ++i) {
    TCHAR buf[MAX_PATH];
    if (DragQueryFile(drop, i, buf, MAX_PATH)) {
      if (PathIsDirectory(buf)) {
        addDirectory(buf);
      }
    }
  }
  DragFinish(drop);
  return 0;
}

void UploadPage::write() {
  PropPage::write(items);
  if (SETTING(SLOTS) > 500)  {
    settings->set(SettingsManager::SLOTS, 500);
  }
  if (SETTING(EXTRA_SLOTS) < 3) {
    settings->set(SettingsManager::EXTRA_SLOTS, 3);
  }
  if (ft.IsDirty()) {
	  ShareManager::getInstance()->shareDownloads();
	  ShareManager::getInstance()->setDirty();
  }
}

#if 0
// Refresh the share. This is a blocking refresh. Might cause problems?
// Hopefully people won't click the checkbox enough for it to be an issue. :-)
ShareManager::getInstance()->setDirty();
ShareManager::getInstance()->refresh(true, false, false);
// Display the new total share size
ctrlTotal.SetWindowText(Util::formatBytesW(ShareManager::getInstance()->getShareSize()).c_str());
#endif

void UploadPage::addDirectory(const tstring& aPath) {
  tstring path = aPath;
  if (path[path.length() - 1] != _T('\\')) {
    path += _T('\\');
  }
  try {
    ShareManager* shareManager = ShareManager::getInstance();
    LineDlg virt;
    virt.title = TSTRING(VIRTUAL_NAME);
    virt.description = TSTRING(VIRTUAL_NAME_LONG);
    virt.line = Text::toT(shareManager->validateVirtual(Util::getLastDir(Text::fromT(path))));
    if (virt.DoModal(m_hWnd) == IDOK) {
      shareManager->addDirectory(Text::fromT(path), Text::fromT(virt.line));
      ctrlTotal.SetWindowText(Util::formatBytesW(shareManager->getShareSize()).c_str());
    }
  } 
  catch(const ShareException& e) {
    MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
  }
}

/**
 * @file
 * $Id: UploadPage.cpp,v 1.9 2008/03/10 06:57:19 alexey Exp $
 */
