#pragma once
#include "FlatIconButton.h"
#include "CaptionFont.h"
#include "TextRenderer.h"
#include "PictureExWnd.h"

class HubInfoBlock : public CWindowImpl<HubInfoBlock>, private TextRenderer::Paragraph {
private:
  friend class HubInfoPanel;
  /* возврашает высоту окна */
  int getHeight() const;
  /* обновляет высоту и координаты окна. модифицирует высоту. возвращает true, если есть изменения */
  bool updateLayout(CDCHandle dc, int paragraphWidth, int x, int& y, bool invalidateAnyway);
public:
  BEGIN_MSG_MAP(HubInfoBlock)
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
  END_MSG_MAP();
    
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};

class HubInfoPanelController {
public:
  virtual Client* getClient() = 0;
};

class HubInfoPanel : public CWindowImpl<HubInfoPanel> {
public:
  HWND Create(HWND hWndParent);
  void updateData();
  int updateLayout(int blocks, bool invalidateAnyway);
  int updateLayout(const CRect& r, int blocks, bool invalidateAnyway);
  void createBanner() { startTimer(); }
  void destroyBanner() { startTimer(); }
  int getBannerWidth() const;
  int getMinHeight() const;
public:
  HubInfoPanel(HubInfoPanelController *controller);

  BEGIN_MSG_MAP(HubInfoPanel)
    MESSAGE_HANDLER(WM_CREATE, onCreate);
    MESSAGE_HANDLER(WM_SIZE, onSize);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
	MESSAGE_HANDLER(WM_TIMER, onTimer);
    COMMAND_HANDLER(ID_FILE_RECONNECT, BN_CLICKED, onForwardCommandToParent);
    COMMAND_HANDLER(ID_FILE_SETTINGS, BN_CLICKED, onForwardCommandToMain);
    COMMAND_HANDLER(IDC_OPEN_MY_LIST, BN_CLICKED, onForwardCommandToMain);
    COMMAND_HANDLER(IDC_REFRESH_FILE_LIST, BN_CLICKED, onForwardCommandToMain);
    COMMAND_HANDLER(IDC_FILELIST_ADD_FILE, BN_CLICKED, onForwardCommandToMain);
    REFLECT_NOTIFICATIONS();
  END_MSG_MAP();

  LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onForwardCommandToParent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/);
  LRESULT onForwardCommandToMain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/);

private:
  HubInfoPanelController* m_controller;
  
  bool m_bannerCreateTimerStarted;
  
  HubInfoBlock pMyNick;
  TextRenderer::WordKey myNick;

  HubInfoBlock pStatus;
  TextRenderer::WordKey statusName;
  TextRenderer::WordKey statusMode;
  
  HubInfoBlock pMyShare;
  TextRenderer::WordKey myShareSize;
  TextRenderer::WordKey mySharePercent;

  CaptionFont greenFont;
  CaptionFont captionFont;
  CaptionFont textFont;
  CaptionFont boldFont;
  CaptionFont buttonFont;

  FlatIconButton m_btnReconnect;
  FlatIconButton m_btnConfigure;
  FlatIconButton m_btnAddFiles;
  FlatIconButton m_btnOpenFileList;
  FlatIconButton m_btnRefreshFileList;

  CPictureExWnd i_banner;
  bool showBanner;
  void createBannerNow();
  void destroyBannerNow();
  void startTimer();
};

class HubInfoNumberPanel : public CWindowImpl<HubInfoNumberPanel>, private TextRenderer::TextBlock {
private:
	enum {
		H_PADDING = 8,
		V_PADDING = 8
	};
public:
	HWND Create(HWND hWndParent);
	void updateData();
	int doLayout(int paragraphWidth) {
		return TextRenderer::TextBlock::doLayout(m_hWnd, paragraphWidth) + 2 * V_PADDING;
	}
public:
	HubInfoNumberPanel(HubInfoPanelController *controller);

	BEGIN_MSG_MAP(HubInfoPanel)
		MESSAGE_HANDLER(WM_CREATE, onCreate);
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
		MESSAGE_HANDLER(WM_PAINT, onPaint);
		REFLECT_NOTIFICATIONS();
	END_MSG_MAP();

	LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
	LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	HubInfoPanelController* m_controller;

	int m_headerHeight;

	TextRenderer::WordKey totalUserCount;
	TextRenderer::WordKey totalSharedSize;

	CaptionFont greenFont;
	CaptionFont textFont;
};
