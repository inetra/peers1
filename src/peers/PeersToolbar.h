#pragma once
#include "../client/SearchManager.h"
#include "TransparentBitmap.h"
#include "FlatButton.h"
#include "FlatIconButton.h"
#include "EditWithIcon.h"
#include "StaticAccessSingleton.h"

class PeersToolbarListener {
public:
  virtual void onToolbarSearchToggle() = 0;
};

class PeersToolbarButton: public FlatButton {
private:
	CBitmap m_image;
	CBitmap m_hoverImage;
	int m_width;
	int m_height;
protected:
	virtual void drawBackrgound(CDCHandle /*dc*/, CRect& /*r*/) { }
	virtual void drawButton(CDCHandle dc, const CRect &r, bool selected);
public:
	BEGIN_MSG_MAP(PeersToolbarButton)
		MESSAGE_HANDLER(WM_SETCURSOR, onSetCursor);
		CHAIN_MSG_MAP(FlatButton);
	END_MSG_MAP();

	PeersToolbarButton() {
		setLeftPadding(0);
		m_width = 50;
		m_height = 50;
	}
	HWND Create(ATL::_U_STRINGorID resourceId, ATL::_U_STRINGorID hoverResourceId, HWND hWndParent, _U_MENUorID MenuOrID);
	virtual SIZE getPreferredSize();
	LRESULT onSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

class PeersToolbarSearchPrompt : public CWindowImpl<PeersToolbarSearchPrompt> {
  BEGIN_MSG_MAP(PeersToolbar)
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
  END_MSG_MAP();

  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
public:
  PeersToolbarSearchPrompt(): m_leftPadding(2), m_backgroundColor(0) { }
  int selectWidth() const;
  void setLeftPadding(int value) { m_leftPadding = value; }
  void setBackgroundColor(COLORREF value) { m_backgroundColor = value; }
private:
  COLORREF m_backgroundColor;
  int m_leftPadding;
};

class CSearchEdit : public CEditWithIcon {
public:
  CSearchEdit(ATL::_U_STRINGorID resourceId): CEditWithIcon(resourceId) { }
  BEGIN_MSG_MAP(CSearchEdit)
    MESSAGE_HANDLER(WM_KEYDOWN, onKeyDown);
    MESSAGE_HANDLER(WM_CHAR, onChar);
    CHAIN_MSG_MAP(CEditWithIcon);
  END_MSG_MAP();

  LRESULT onKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

class CFileTypeComboBox : public CWindowImpl<CFileTypeComboBox, CComboBox> {
private:
  static ResourceManager::Strings searchTypeNames[];
public:
  BEGIN_MSG_MAP(CFileTypeComboBox)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_DESTROY, onDestroy);
    MESSAGE_HANDLER(OCM_DRAWITEM, onDrawItem);
    MESSAGE_HANDLER(OCM_MEASUREITEM, onMeasure);
  END_MSG_MAP();
  /* возвращает дополнительную ширину (кроме текста) */
  int getExtraItemWidth() const;
  int calculateItemHeight() const;
private:
  LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onMeasure(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  CImageList searchTypes;
};

class PeersToolbar : public CWindowImpl<PeersToolbar> {
  BEGIN_MSG_MAP(PeersToolbar)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_SIZE, onSize);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);

    COMMAND_HANDLER(IDC_FINISHED, BN_CLICKED, onOpenWindow);
    COMMAND_HANDLER(IDC_FINISHED_UL, BN_CLICKED, onOpenWindow);
    COMMAND_HANDLER(IDC_QUEUE, BN_CLICKED, onOpenWindow);
    COMMAND_HANDLER(IDC_UPLOAD_QUEUE, BN_CLICKED, onOpenWindow);
	COMMAND_HANDLER(IDC_ADVICE_WINDOW, BN_CLICKED, onOpenWindow);
	COMMAND_HANDLER(IDC_SUBSCRIPTIONS, BN_CLICKED, onOpenWindow);
    COMMAND_HANDLER(IDC_PEERS_TOOLBAR_CHAT, BN_CLICKED, onForwardCommandToParent);
    COMMAND_HANDLER(IDC_PEERS_TOOLBAR_EGOPHONE, BN_CLICKED, onForwardCommandToParent);

    REFLECT_NOTIFICATIONS();
  END_MSG_MAP();

  LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onEraseBkGnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onOpenWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onForwardCommandToParent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
    return ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(wID, wNotifyCode), (LPARAM) hWndCtl);
  }
public:
  PeersToolbar();
  HWND Create(HWND hWndParent);
  int getHeight() const { return 86; }
private:
  PeersToolbarButton m_btnFinishedDownloads;
  PeersToolbarButton m_btnDownloadQueue;
  PeersToolbarButton m_btnAdvice;
  PeersToolbarButton m_btnSubscriptions;
  PeersToolbarButton m_btnChat;
};
