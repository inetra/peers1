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
#include "PropPage.h"
#include "../peers/CaptionFont.h"
#include "../peers/ControlAdjuster.h"

#define SETTING_STR_MAXLEN 1024

void PropPage::read(Item const* items) {
  const HWND page = getPageWindow();
  dcassert(page != NULL);

  bool const useDef = true;
  for(Item const* i = items; i->type != T_END; i++)
  {
    switch(i->type)
    {
    case T_STR:
#if DIM_EDIT_EXPERIMENT
      CDimEdit* tempCtrl = new CDimEdit;
      tempCtrl->SubclassWindow(GetDlgItem(page, i->itemID));
      tempCtrl->SetDimText(Text::toT(settings->get((SettingsManager::StrSetting)i->setting, true)));
      tempCtrl->SetDimColor(RGB(192, 192, 192));
      ctrlMap[i->itemID] = tempCtrl;
#endif
      if (GetDlgItem(page, i->itemID) == NULL) {
        // Control not exist ? Why ??
        throw;
      }
      ::SetDlgItemText(page, i->itemID,
        Text::toT(settings->get((SettingsManager::StrSetting)i->setting, useDef)).c_str());
      break;
    case T_INT:

      if (GetDlgItem(page, i->itemID) == NULL) {
        // Control not exist ? Why ??
        throw;
      }
      ::SetDlgItemInt(page, i->itemID,
        settings->get((SettingsManager::IntSetting)i->setting, useDef), FALSE);
      break;
    case T_INT64:
      if(!settings->isDefault(i->setting)) {
        tstring s = Util::toStringW(settings->get((SettingsManager::Int64Setting)i->setting, useDef));
        ::SetDlgItemText(page, i->itemID, s.c_str());
      }
      break;
	case T_BOOL:
	case T_BOOL_INVERTED:
      if (GetDlgItem(page, i->itemID) == NULL) {
        // Control not exist ? Why ??
        throw;
      }
	  bool value = settings->getBool((SettingsManager::IntSetting)i->setting, useDef);
	  if (i->type == T_BOOL_INVERTED) {
		  value = !value;
	  }
	  ::CheckDlgButton(page, i->itemID, value ? BST_CHECKED : BST_UNCHECKED);
    }
  }
}

void PropPage::readCheckBoxList(ListItem* listItems, HWND list) {
  dcassert(listItems && list);
  CListViewCtrl ctrl(list);
  CRect rc;
  ctrl.GetClientRect(rc);
  ctrl.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
  ctrl.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, rc.Width(), 0);

  LVITEM lvi;
  lvi.mask = LVIF_TEXT;
  lvi.iSubItem = 0;

  for(int i = 0; listItems[i].setting != 0; i++) {
    lvi.iItem = i;
    lvi.pszText = const_cast<TCHAR*>(CTSTRING_I(listItems[i].desc));
    ctrl.InsertItem(&lvi);
    ctrl.SetCheckState(i, settings->getBool(SettingsManager::IntSetting(listItems[i].setting), true));
  }
  ctrl.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

void PropPage::write(Item const* items) {
  HWND page = getPageWindow();
  dcassert(page != NULL);

  TCHAR buf[SETTING_STR_MAXLEN];
  for(Item const* i = items; i->type != T_END; i++)
  {
    switch(i->type)
    {
    case T_STR:
      {
        if (GetDlgItem(page, i->itemID) == NULL) {
          // Control not exist ? Why ??
          throw;
        }
        ::GetDlgItemText(page, i->itemID, buf, SETTING_STR_MAXLEN);
        settings->set((SettingsManager::StrSetting)i->setting, Text::fromT(tstring(buf)));
#if DIM_EDIT_EXPERIMENT
        if (ctrlMap[i->itemID]) {
          ctrlMap[i->itemID]->UnsubclassWindow();
          delete ctrlMap[i->itemID];
          ctrlMap.erase(i->itemID);
        }
#endif
        break;
      }
    case T_INT:
      {
        if (GetDlgItem(page, i->itemID) == NULL) {
          // Control not exist ? Why ??
          throw;
        }

        ::GetDlgItemText(page, i->itemID, buf, SETTING_STR_MAXLEN);
        settings->set((SettingsManager::IntSetting)i->setting, Util::toInt(Text::fromT(tstring(buf))));
        break;
      }
    case T_INT64:
      {
        ::GetDlgItemText(page, i->itemID, buf, SETTING_STR_MAXLEN);
        settings->set((SettingsManager::Int64Setting)i->setting, Text::fromT(tstring(buf)));
        break;
      }
    case T_BOOL:
	case T_BOOL_INVERTED:
      {
        if (GetDlgItem(page, i->itemID) == NULL) {
          // Control not exist ? Why ??
          throw;
        }
		bool value = ::IsDlgButtonChecked(page, i->itemID) == BST_CHECKED;
		if (i->type == T_BOOL_INVERTED) {
			value = !value;
		}
		settings->set((SettingsManager::IntSetting)i->setting, value);
      }
    }
  }
}

void PropPage::writeCheckBoxList(ListItem* listItems, HWND list) {
  dcassert(listItems && list);
  CListViewCtrl ctrl(list);
  for(int i = 0; listItems[i].setting != 0; i++) {
    settings->set(SettingsManager::IntSetting(listItems[i].setting), ctrl.GetCheckState(i));
  }
}

void PropPage::translate(TextItem* textItems) {
  const HWND page = getPageWindow();
  dcassert(page != NULL);
  if (textItems != NULL) {
    for (int i = 0; textItems[i].itemID != 0; ++i) {
      if (textItems[i].translatedStringFactory != NULL) {
        ::SetDlgItemText(page, textItems[i].itemID, (*textItems[i].translatedStringFactory)().c_str());
      }
      else {
        ::SetDlgItemText(page, textItems[i].itemID, CTSTRING_I(textItems[i].translatedString));
      }
#ifdef _DEBUG
      if (!GetDlgItem(page, textItems[i].itemID)) {
        dcdebug("translate - control[%d] %d = %s is not found\n", i, textItems[i].itemID, CSTRING_I(textItems[i].translatedString));
      }
#endif
      if (textItems[i].autoSize != AUTOSIZE_NONE) {
        const HWND controlWnd = GetDlgItem(page, textItems[i].itemID);
        dcassert(controlWnd);
        if (isStaticControl(controlWnd)) {
          CStatic staticWnd(controlWnd);
          RECT staticRect;
          staticWnd.GetWindowRect(&staticRect);
          CWindow(staticWnd.GetParent()).ScreenToClient(&staticRect);
          if (textItems[i].autoSize == AUTOSIZE_VERT) {
            const SIZE labelSize = ControlAdjuster::adjustStaticSize(staticWnd, true);
            if (labelSize.cy > staticRect.bottom - staticRect.top) {
              staticWnd.MoveWindow(staticRect.left, 
                (staticRect.top + staticRect.bottom - labelSize.cy) / 2,
                labelSize.cx,
                labelSize.cy);
            }
          }
          else {
            dcassert(textItems[i].autoSize == AUTOSIZE_HORIZ || textItems[i].autoSize == AUTOSIZE_HORIZ_ALWAYS);
            const SIZE labelSize = ControlAdjuster::adjustStaticSize(staticWnd);
            if (textItems[i].autoSize == AUTOSIZE_HORIZ_ALWAYS || labelSize.cx > staticRect.right - staticRect.left) {
              staticWnd.MoveWindow(staticRect.left, staticRect.top, labelSize.cx, labelSize.cy);
            }
          }
        }
      }
    }
  }
}

void PropPage::initHeader(int headerId) {
  const HWND page = getPageWindow();
  dcassert(page != NULL);
  if (!captionFont) {
    captionFont.Attach(CaptionFont().Detach());
  }
  CWindow headerWnd(GetDlgItem(page, headerId));
  dcassert(headerWnd);
  headerWnd.SetFont(captionFont);
  if (isStaticControl(headerWnd)) {
    dcassert(sizeof(CStatic) == 4);
    const SIZE labelSize = ControlAdjuster::adjustStaticSize(CStatic(headerWnd));
    RECT labelRect;
    headerWnd.GetWindowRect(&labelRect);
    CWindow(headerWnd.GetParent()).ScreenToClient(&labelRect);
    headerWnd.MoveWindow(labelRect.left, labelRect.top, labelSize.cx, labelSize.cy);
  }
}

bool PropPage::isStaticControl(HWND controlWnd) const {
  TCHAR szBuf[32];
  return GetClassName(controlWnd, szBuf, COUNTOF(szBuf)) && lstrcmp(szBuf, WC_STATIC) == 0;
}

/**
 * @file
 * $Id: PropPage.cpp,v 1.11.2.1 2008/10/16 16:51:19 alexey Exp $
 */
