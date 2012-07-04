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
#include "PropertiesDlg.h"
#include "GeneralPage.h"
#include "DownloadPage.h"
#include "UploadPage.h"
#include "AppearancePage.h"
#include "AdvancedPage.h"
#include "LogPage.h"
#include "SoundsPage.h"
#include "UCPage.h"
#include "LimitPage.h"
//[-]PPA [Doxygen 1.5.1] #include "PropPageTextStyles.h"
#include "FakeDetect.h"
#include "AVIPreview.h"
#include "OperaColorsPage.h"
//[-]PPA #include "ClientsPage.h"
#include "ToolbarPage.h"
#include "FavoriteDirsPage.h"
#include "Popups.h"
#include "SDCPage.h"
#include "UserListColours.h"
#include "NetworkPage.h"
#include "WindowsPage.h"
#include "QueuePage.h"
//[-]PPA #include "CertificatesPage.h"
#include "MiscPage.h"
#include "MessagesPage.h" // !SMT!-PSW
#include "CFlylinkIPFilter.h" //[+]PPA
#include "../peers/MagnetPage.h" //[+]PPA
#include "../peers/SearchSettingsPage.h"
#include "../peers/PeersVersion.h"

bool PropertiesDlg::needUpdate = false;

PropertiesDlg::PropertiesDlg(HWND parent, SettingsManager* s):
m_expertMode(false),
m_busy(false),
m_lastPage(0),
m_settingsManager(s),
TreePropertySheet(CTSTRING(SETTINGS), 0, parent)
{
  // Hide "Apply" button
  m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
  m_psh.dwFlags &= ~PSH_HASHELP;
}

void PropertiesDlg::initPages() {
  pages.push_back(new GeneralPage());
  pages.push_back(new NetworkPage());
  pages.push_back(new UploadPage());
  pages.push_back(new DownloadPage());
  pages.push_back(new AppearancePage());
  pages.push_back(new PropPageTextStyles());
  pages.push_back(new UserListColours());
  pages.push_back(new OperaColorsPage());
  pages.push_back(new Popups());
  pages.push_back(new SoundsPage());
  pages.push_back(new ToolbarPage());
  pages.push_back(new WindowsPage());
  pages.push_back(new AdvancedPage());
  pages.push_back(new SDCPage());
  pages.push_back(new LogPage());
  pages.push_back(new UCPage());
  pages.push_back(new FavoriteDirsPage());
  pages.push_back(new AVIPreview());	
  pages.push_back(new QueuePage());
  pages.push_back(new LimitPage());
  pages.push_back(new FakeDetect());	
  pages.push_back(new MessagesPage()); // !SMT!-PSW
  pages.push_back(new CFlylinkIPFilter()); //[+]PPA
  //[-]PPA pages[n++] = new ClientsPage(s);	
  //[-]PPA pages[n++] = new CertificatesPage(s);	
  pages.push_back(new MiscPage());
  pages.push_back(new SearchSettingsPage());
  pages.push_back(new MagnetPage());
}

INT_PTR PropertiesDlg::DoModal(HWND hWndParent) {
  initPages();
  for (PropPageVector::iterator i = pages.begin(); i != pages.end(); ++i) {
    (*i)->setContext(this);
  }
  for (PropPageVector::iterator i = pages.begin(); i != pages.end(); ++i) {
    AddPage((*i)->getPSP());
  }
  return TreePropertySheet::DoModal(hWndParent);
}

PropertiesDlg::~PropertiesDlg() {
}

void PropertiesDlg::write() {
  int index = 0;
  for (PropPageVector::iterator i = pages.begin(); i != pages.end(); ++i) {
    const HWND pageWnd = PropSheet_IndexToHwnd((HWND)*this, index);
    if (pageWnd) {
      (*i)->write();
    }
    ++index;
  }
  for (PostSaveParticipantMap::iterator i = m_saveParticipants.begin(); i != m_saveParticipants.end(); ++i) {
	  (*(i->second))();
  }
}

LRESULT PropertiesDlg::onOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
  int index = 0;
  for (PropPageVector::iterator i = pages.begin(); i != pages.end(); ++i) {
    const HWND pageWnd = PropSheet_IndexToHwnd((HWND)*this, index);
    if (pageWnd) {
      try {
        (*i)->validation();
      }
      catch (BasePropPageValidationException* e) {
        const HTREEITEM root = ctrlTree.GetRootItem();
        const HTREEITEM pageNode = findItem(index, root);
        ctrlTree.SelectItem(pageNode ? pageNode : root);
        if (e->getControlId()) {
          HWND childWnd = ::GetDlgItem(pageWnd, e->getControlId());
          if (childWnd) {
            ::SetFocus(childWnd);
          }
        }
        MessageBox(e->getMessage().c_str(), (tstring(_T(APPNAME)) + _T(" ") + TSTRING(SETTINGS)).c_str(), MB_ICONWARNING | MB_OK);
        delete e;
        return 0;
      }
    }
    ++index;
  }
  write();
  bHandled = FALSE;
  return TRUE;
}

void PropertiesDlg::addTree() {
  TreePropertySheet::addTree();
  CRect r;
  ctrlTree.GetWindowRect(r);
  ScreenToClient(r);
  CRect clientRect;
  GetClientRect(clientRect);
  CRect checkRect(r.left, r.bottom, r.right, clientRect.bottom);
  checkExpertMode.Create(m_hWnd, checkRect, CTSTRING(SETTINGS_CHECKBOX_EXPERT_MODE), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_AUTOCHECKBOX, 0, IDC_SETTINGS_EXPERT_MODE);
  checkExpertMode.SetFont(WinUtil::systemFont);
  if (BOOLSETTING(SETTINGS_EXPERT_MODE)) {
    checkExpertMode.SetCheck(BST_CHECKED);
    m_expertMode = true;
  }
}

void PropertiesDlg::fillTree() {
  TreePropertySheet::fillTree();
  const HTREEITEM root = ctrlTree.GetRootItem();
  const HTREEITEM pageNode = findItem(SETTING(SETTINGS_STATE) ? SETTING(PAGE) : m_lastPage, root);
  ctrlTree.SelectItem(pageNode ? pageNode : root);
}

void PropertiesDlg::onPageChanged(int pageIndex) {
  if (!m_busy) {
    m_lastPage = pageIndex;
    pages[pageIndex]->onActivate();
    if (SETTING(SETTINGS_STATE)) {
      SettingsManager::getInstance()->set(SettingsManager::PAGE, pageIndex);
    }
  }
}

bool PropertiesDlg::isVisiblePage(int pageIndex) {
  if (m_expertMode) {
    return true;
  }
  return pageIndex >= 0 && pageIndex < (int) pages.size() && !pages[pageIndex]->isExpertModePage();
}

LRESULT PropertiesDlg::onExpertModeToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  m_expertMode = checkExpertMode.GetCheck() == BST_CHECKED;
  SettingsManager::getInstance()->set(SettingsManager::SETTINGS_EXPERT_MODE, m_expertMode);
  m_busy = true;
  ctrlTree.DeleteAllItems();
  m_busy = false;
  fillTree();
  ctrlTree.Invalidate();
  int index = 0;
  for (PropPageVector::iterator i = pages.begin(); i != pages.end(); ++i) {
    PropPage* page = &**i;
    if (!page->isExpertModePage()) {
      const HWND pageWnd = PropSheet_IndexToHwnd((HWND)*this, index);
      if (pageWnd) {
        page->onExpertModeToggle();
      }
    }
    ++index;
  }
  return 0;
}

int PropertiesDlg::getItemImageIndex(int page) {
  return pages[page]->getImageIndex();
}

/**
 * @file
 * $Id: PropertiesDlg.cpp,v 1.15.2.1 2008/10/12 12:37:47 alexey Exp $
 */
