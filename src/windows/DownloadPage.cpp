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
#include "DownloadPage.h"
#include "../client/ShareManager.h"
#include "../peers/ShareManagerRefreshParticipant.h"

PropPage::TextItem DownloadPage::texts[] = {
  { IDC_DOWNLOADPAGE_TITLE, ResourceManager::SETTINGS_DOWNLOADS },
  { IDC_DOWNLOADPAGE_SUBTITLE, ResourceManager::SETTINGS_DOWNLOADS_SUBTITLE }, 
  { IDC_SETTINGS_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_DOWNLOAD_DIRECTORY },
  { IDC_BROWSEDIR, ResourceManager::BROWSE_ACCEL },
  { IDC_SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY }, 
  { IDC_BROWSETEMPDIR, ResourceManager::BROWSE }, 
  { IDC_SETTINGS_DOWNLOAD_LIMITS, ResourceManager::SETTINGS_DOWNLOAD_LIMITS },
  { IDC_SETTINGS_DOWNLOADS_MAX, ResourceManager::SETTINGS_DOWNLOADS_MAX, true },
  { IDC_SETTINGS_FILES_MAX, ResourceManager::SETTINGS_FILES_MAX, true },
  { IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE, ResourceManager::SETTINGS_DOWNLOADS_SPEED_PAUSE, true },
  { IDC_SETTINGS_SPEEDS_NOT_ACCURATE, ResourceManager::SETTINGS_SPEEDS_NOT_ACCURATE },
  { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item DownloadPage::items[] = {
  { IDC_TEMP_DOWNLOAD_DIRECTORY, SettingsManager::TEMP_DOWNLOAD_DIRECTORY, PropPage::T_STR },
  { IDC_DOWNLOADDIR,	SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR }, 
  { IDC_DOWNLOAD_DIRECTORY_SHORTCUT, SettingsManager::DOWNLOAD_DIRECTORY_SHORTCUT, PropPage::T_BOOL },
  { IDC_DOWNLOADS, SettingsManager::DOWNLOAD_SLOTS, PropPage::T_INT },
  { IDC_FILES, SettingsManager::FILE_SLOTS, PropPage::T_INT },
  { IDC_MAXSPEED, SettingsManager::MAX_DOWNLOAD_SPEED, PropPage::T_INT },
  { 0, 0, PropPage::T_END }
};

LRESULT DownloadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  PropPage::translate(texts);
  initHeader(IDC_DOWNLOADPAGE_TITLE);
  PropPage::read(items);
  updateDirectoryControlsLayout();

  CUpDownCtrl(GetDlgItem(IDC_FILESPIN)).SetRange32(0, 100);
  CUpDownCtrl(GetDlgItem(IDC_SLOTSSPIN)).SetRange32(0, 100);
  CUpDownCtrl(GetDlgItem(IDC_SPEEDSPIN)).SetRange32(0, 10000);
  return TRUE;
}

void DownloadPage::updateDirectoryControlsLayout() {
	CRect dlgRect;
	GetClientRect(dlgRect);
	CRect r;
	GetDlgItem(IDC_BROWSEDIR).GetWindowRect(r);
	ScreenToClient(r);
	const int buttonLeft = dlgRect.right - r.Width() - 8;
	r.MoveToX(buttonLeft);
	GetDlgItem(IDC_BROWSEDIR).MoveWindow(r);
	//
	GetDlgItem(IDC_DOWNLOADDIR).GetWindowRect(r);
	ScreenToClient(r);
	r.right = buttonLeft - 8;
	GetDlgItem(IDC_DOWNLOADDIR).MoveWindow(r);
	//
	GetDlgItem(IDC_BROWSETEMPDIR).GetWindowRect(r);
	ScreenToClient(r);
	r.MoveToX(buttonLeft);
	GetDlgItem(IDC_BROWSETEMPDIR).MoveWindow(r);
	//
	GetDlgItem(IDC_TEMP_DOWNLOAD_DIRECTORY).GetWindowRect(r);
	ScreenToClient(r);
	r.right = buttonLeft - 8;
	GetDlgItem(IDC_TEMP_DOWNLOAD_DIRECTORY).MoveWindow(r);
}

void DownloadPage::onExpertModeToggle() {
  static const int expertControlIds[] = {
    IDC_SETTINGS_DOWNLOAD_LIMITS,
    
    IDC_DOWNLOADS,
    IDC_SLOTSSPIN,
    IDC_SETTINGS_FILES_MAX,

    IDC_FILES,
    IDC_FILESPIN,
    IDC_SETTINGS_DOWNLOADS_MAX,

    IDC_MAXSPEED,
    IDC_SPEEDSPIN,
    IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE,

    IDC_SETTINGS_SPEEDS_NOT_ACCURATE
  };
  ShowWindows(expertControlIds, isExpertMode());
}

void DownloadPage::write() {	
  PropPage::write(items);
  const string& s = SETTING(DOWNLOAD_DIRECTORY);
  if (s.length() > 0 && s[s.length() - 1] != '\\') {
    settings->set(SettingsManager::DOWNLOAD_DIRECTORY, s + '\\');
  }
  const string& t = SETTING(TEMP_DOWNLOAD_DIRECTORY);
  if (t.length() > 0 && t[t.length() - 1] != '\\') {
    settings->set(SettingsManager::TEMP_DOWNLOAD_DIRECTORY, t + '\\');
  }
  if (ShareManager::getInstance()->shareDownloads()) {
	addPostSaveParticipant(ShareManagerRefreshParticipant::NAME, ShareManagerRefreshParticipant::execute);
  }
}

void DownloadPage::browseDirectory(int controlId) {
  tstring dir = GetDlgItemString(controlId);
  if (WinUtil::browseDirectory(dir, m_hWnd)) {
    // Adjust path string
    if (dir.size() > 0 && dir[dir.size() - 1] != '\\') {
      dir += '\\';
    }
    SetDlgItemText(controlId, dir.c_str());
  }
}


LRESULT DownloadPage::onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  browseDirectory(IDC_DOWNLOADDIR);
  return 0;
}

LRESULT DownloadPage::onClickedBrowseTempDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  browseDirectory(IDC_TEMP_DOWNLOAD_DIRECTORY);
  return 0;
}

LRESULT DownloadPage::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  updateDirectoryControlsLayout();
  return 0;
}

/**
 * @file
 * $Id: DownloadPage.cpp,v 1.5.2.2 2008/12/08 19:26:48 alexey Exp $
 */
