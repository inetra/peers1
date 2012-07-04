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

#if !defined(PROPERTIES_DLG_H)
#define PROPERTIES_DLG_H

#include "PropPage.h"
#include "TreePropertySheet.h"

class PropertiesDlg : public TreePropertySheet, PropPageContext {
public:
  BEGIN_MSG_MAP(PropertiesDlg)
    COMMAND_ID_HANDLER(IDOK, onOK)
    COMMAND_HANDLER(IDC_SETTINGS_EXPERT_MODE, BN_CLICKED, onExpertModeToggle)
    CHAIN_MSG_MAP(TreePropertySheet)
    ALT_MSG_MAP(TreePropertySheet::TAB_MESSAGE_MAP)
    CHAIN_MSG_MAP_ALT(TreePropertySheet, TreePropertySheet::TAB_MESSAGE_MAP)
  END_MSG_MAP()

  PropertiesDlg(HWND parent, SettingsManager* s);
  ~PropertiesDlg();

  INT_PTR DoModal(HWND hWndParent);

  static bool needUpdate;

  LRESULT onOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT onExpertModeToggle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

protected:
  virtual void initPages();
  virtual void addTree();
  virtual void fillTree();
  virtual void onPageChanged(int pageIndex);
  virtual bool isVisiblePage(int pageIndex);
  virtual bool allowChange(NMTREEVIEW*) { return !m_busy; }
  virtual int getItemImageIndex(int page);
  void write();
  // PropPageContext
  virtual bool isExpertMode() const { return m_expertMode; }
  virtual SettingsManager* getSettingsManager() const { return m_settingsManager; }

  SettingsManager* m_settingsManager;
  typedef vector<Pointer<PropPage>> PropPageVector;
  PropPageVector pages;
private:
  CButton checkExpertMode;
  bool m_expertMode;
  bool m_busy;
  int m_lastPage;
};

#endif // !defined(PROPERTIES_DLG_H)

/**
 * @file
 * $Id: PropertiesDlg.h,v 1.9 2007/12/08 15:12:04 alexey Exp $
 */
