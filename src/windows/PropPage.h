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

#if !defined(PROP_PAGE_H)
#define PROP_PAGE_H

#include "DimEdit.h"
#include "../peers/FastDelegate/FastDelegate.h"

#define SETTINGS_BUF_LEN 1024
#define DIM_EDIT_EXPERIMENT 0

#include "../client/Pointer.h"

template <WORD t_wDlgTemplateID> class PeersPropertyPage : public CPropertyPage<t_wDlgTemplateID> {
protected:
  template <size_t size> void EnableWindows(const int (&controlIds)[size], bool bEnable) {
    for (int i = 0; i < size; ++i) {
      const HWND controlWnd = GetDlgItem(controlIds[i]);
      dcassert(controlWnd != NULL);
      ::EnableWindow(controlWnd, bEnable);
    }
  }
  template <size_t size> void ShowWindows(const int (&controlIds)[size], bool bShow) {
    for (int i = 0; i < size; ++i) {
      const HWND controlWnd = GetDlgItem(controlIds[i]);
      dcassert(controlWnd != NULL);
      ::ShowWindow(controlWnd, bShow ? SW_SHOW : SW_HIDE);
    }
  }
  template <size_t size> void layoutControls(const int (&controlIds)[size]) {
    dcassert(size > 1);
    RECT r;
    GetDlgItem(controlIds[0]).GetWindowRect(&r);
    ScreenToClient(&r);
    for (int i = 1; i < size; ++i) {
      r.right += 8;
      CWindow wnd(GetDlgItem(controlIds[i]));
      RECT nextRect;
      wnd.GetWindowRect(&nextRect);
      ScreenToClient(&nextRect);
      const int width = nextRect.right - nextRect.left;
      wnd.MoveWindow(r.right, nextRect.top, width, nextRect.bottom - nextRect.top);
      r.right += width;
    }
  }
  tstring GetDlgItemString(int controlId) {
    const HWND controlWnd = GetDlgItem(controlId);
    if (controlWnd) {
      CWindow wnd(controlWnd);
      const int textLen = wnd.GetWindowTextLength();
      if (textLen > 0) {
        AutoArray<TCHAR> buf(textLen + 1);
        wnd.GetWindowText(buf, textLen + 1);
        return (TCHAR*) buf;
      }
    }
    return Util::emptyStringT;
  }
  virtual DLGPROC GetDialogProc() {
    return PeersDialogProc;
  }
  static INT_PTR CALLBACK PeersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    INT_PTR result = CPropertyPage::DialogProc(hWnd, uMsg, wParam, lParam);
    if (uMsg == WM_INITDIALOG) {
      CPropertyPage* pThis = (CPropertyPage*) hWnd;
      PropPage* page = dynamic_cast<PropPage*>(pThis);
      if (page && !page->isExpertModePage()) {
        page->onExpertModeToggle();
      }
    }
    return result;
  }
public:
  PeersPropertyPage() : CPropertyPage<t_wDlgTemplateID>() {
    m_psp.dwFlags |= PSP_RTLREADING;
  }
};

using namespace fastdelegate;

typedef FastDelegate1<SettingsManager::IntSetting,int> CurrentIntSettingDelegate;
typedef void (*PostSaveParticipant)();

class ATL_NO_VTABLE PropPageContext {
private:
  typedef map<SettingsManager::IntSetting,CurrentIntSettingDelegate> CurrentIntDelegateMap;
  CurrentIntDelegateMap m_delegates;
protected:
  typedef map<string,PostSaveParticipant> PostSaveParticipantMap;
  PostSaveParticipantMap m_saveParticipants;
public:
  virtual bool isExpertMode() const = 0;

  virtual SettingsManager* getSettingsManager() const = 0;

  virtual int getCurrentSetting(SettingsManager::IntSetting setting) const {
    CurrentIntDelegateMap::const_iterator result = m_delegates.find(setting);
    if (result == m_delegates.end()) {
      return getSettingsManager()->get(setting);
    }
    else {
      return result->second(setting);
    }
  }

  void setCurrentSettingDelegate(SettingsManager::IntSetting setting, CurrentIntSettingDelegate d) {
    m_delegates[setting] = d;
  }

  void addPostSaveParticipant(const string& name, PostSaveParticipant participant) {
	  m_saveParticipants[name] = participant;
  }
};

class BasePropPageValidationException {
public:
  virtual tstring getMessage() const = 0;
  virtual int getControlId() const = 0;
  virtual ~BasePropPageValidationException() {}
};

class PropPageValidationException : public BasePropPageValidationException {
private:
  const ResourceManager::Strings m_messageId;
  const int m_controlId;
public:
  PropPageValidationException(ResourceManager::Strings messageId, int controlId): m_messageId(messageId), m_controlId(controlId) { }
  PropPageValidationException(ResourceManager::Strings messageId): m_messageId(messageId), m_controlId(0) { }
  virtual ~PropPageValidationException() {}
  virtual tstring getMessage() const { return WSTRING_I(m_messageId); }
  virtual int getControlId() const { return m_controlId; }
};

class PropPage : public PointerBase {
private:
  CFont captionFont;
  PropPageContext* m_context;
protected:
  bool isStaticControl(HWND controlWnd) const;
public:
  PropPage(): m_context(NULL), settings(NULL) { }
  virtual ~PropPage() { }

  /* сообщает странице о том, что она активизирована */
  virtual void onActivate() { }
  /* возвращает индекс картинки этой страницы в дереве */
  virtual int getImageIndex() const = 0;
  /* возвращает внутреннюю структуру данных этой страницы в диалоге */
  virtual PROPSHEETPAGE *getPSP() = 0;
  /* возвращает идентификатор окна этой страницы */
  virtual HWND getPageWindow() = 0;
  /* тип страницы - доступна только в экспертном режиме или всегда */
  virtual bool isExpertModePage() = 0;
  virtual void write() = 0;
  virtual void validation() throw(BasePropPageValidationException*) { }

  void setContext(PropPageContext* context) {
    m_context = context;
    settings = context->getSettingsManager();
  }

  /* сообщает странице о переключении типа режима, только для страниц которые доступны всегда */
  virtual void onExpertModeToggle() {
  }

  enum Type { T_STR, T_INT, T_BOOL, T_BOOL_INVERTED, T_CUSTOM, T_INT64, T_END };

  struct Item {
    WORD itemID;
    int setting;
    Type type;
  };
  
  struct ListItem {
    int setting;
    ResourceManager::Strings desc;
  };

  enum AutoSizeType { AUTOSIZE_NONE, AUTOSIZE_HORIZ, AUTOSIZE_HORIZ_ALWAYS, AUTOSIZE_VERT };

  class AutoSize {
  private:
    const AutoSizeType type;
  public:
    AutoSize(): type(AUTOSIZE_NONE) { }
    AutoSize(bool /*value*/): type(AUTOSIZE_VERT) { }
    AutoSize(AutoSizeType value): type(value) { }
    operator AutoSizeType () const { return type; }
  };

  struct TextItem {
    WORD itemID;
    ResourceManager::Strings translatedString;
    AutoSize autoSize;
    tstring (*translatedStringFactory)();
  };

protected:
  bool isExpertMode() const {
    return m_context != NULL && m_context->isExpertMode();
  }

  int getCurrentSetting(SettingsManager::IntSetting setting) const {
    dcassert(m_context != NULL);
    return m_context->getCurrentSetting(setting);
  }

  void setCurrentSettingDelegate(SettingsManager::IntSetting setting, CurrentIntSettingDelegate d)  {
    dcassert(m_context != NULL);
    m_context->setCurrentSettingDelegate(setting, d);
  }

  void addPostSaveParticipant(const string& name, PostSaveParticipant participant) {
    dcassert(m_context != NULL);
    m_context->addPostSaveParticipant(name, participant);
  }

#if DIM_EDIT_EXPERIMENT
  std::map<WORD, CDimEdit *> ctrlMap;
#endif
  SettingsManager* settings;
  void read(Item const* items);
  void readCheckBoxList(ListItem* listItems, HWND list);
  void write(Item const* items);
  void writeCheckBoxList(ListItem* listItems, HWND list);
  void translate(TextItem* textItems);
  void initHeader(int headerId);
};

template <class T, int IMAGE_INDEX, bool t_isExpertMode = true> class PropPageImpl : public PropPage {
public:
  PropPageImpl() { }

  virtual int getImageIndex() const { return IMAGE_INDEX; }

  virtual PROPSHEETPAGE *getPSP() {
    return (PROPSHEETPAGE*) *((T*)this);
  }

  virtual HWND getPageWindow() {
     return (HWND) *((T*)this);
  }

  virtual bool isExpertModePage() {
    return t_isExpertMode;
  }
};

#endif // !defined(PROP_PAGE_H)

/**
 * @file
 * $Id: PropPage.h,v 1.16.2.1 2008/10/16 16:51:19 alexey Exp $
 */
