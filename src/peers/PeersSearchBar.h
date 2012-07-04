#pragma once
#include "PeersToolbar.h"
#include "TransparentBitmap.h"

class PeersSearchPrompt : public CWindowImpl<PeersSearchPrompt> {
	BEGIN_MSG_MAP(PeersSearchPrompt)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
		MESSAGE_HANDLER(WM_PAINT, onPaint);
	END_MSG_MAP();

	LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
	LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
public:
	PeersSearchPrompt(): m_leftPadding(2), m_backgroundColor(0) { m_icon.init(IDB_PEERS_TOOLBAR_SEARCH_ICON2); }
	int selectWidth() const;
	void setLeftPadding(int value) { m_leftPadding = value; }
	void setBackgroundColor(COLORREF value) { m_backgroundColor = value; }
private:
	COLORREF m_backgroundColor;
	int m_leftPadding;
	TransparentBitmap m_icon;
};

class PeersSearchBar : public CWindowImpl<PeersSearchBar>, public StaticAccessSingleton<PeersSearchBar> {
	BEGIN_MSG_MAP(PeersSearchBar)
		MESSAGE_HANDLER(WM_CREATE, onCreate);
		MESSAGE_HANDLER(WM_SIZE, onSize);
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
		MESSAGE_HANDLER(WM_PAINT, onPaint);

		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColorStatic);
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtrlColorEdit);
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtrlColorEdit);

		COMMAND_HANDLER(IDC_PEERS_TOOLBAR_SEARCH_TOGGLE, BN_CLICKED, onToggle);
		COMMAND_HANDLER(IDC_PEERS_TOOLBAR_SEARCH_BUTTON, BN_CLICKED, onSearchButton);

		REFLECT_NOTIFICATIONS();
	END_MSG_MAP();

	LRESULT onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onEraseBkGnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT onToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCtrlColorEdit(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
public:
	PeersSearchBar(PeersToolbarListener* listener);
	HWND Create(HWND hWndParent);
	int getHeight() const { return m_expanded ? m_expandedHeight : m_compactHeight; }
	void expand();
	void collapse();
	void setFocusToSearch();
	void setSearchText(const tstring& s);
	void setSearchFileType(SearchManager::TypeModes fileType);
	void setSearchSize(SearchManager::SizeModes sizeMode, int64_t fileSize);
private:
	CBrush m_backgroundBrush;

	PeersToolbarListener* m_listener;

	bool m_expanded;
	int m_compactHeight;
	int m_expandedHeight;

	void createAvancedSearchFields();
	PeersSearchPrompt m_searchPrompt;
	FlatButton m_toggleButton;
	CSearchEdit m_search;
	CButton m_searchButton;
	CFileTypeComboBox ctrlFiletype;
	CStatic typeLabel;
	CStatic sizeLabel;
	CComboBox ctrlMode;
	CEdit ctrlSize;
	CComboBox ctrlSizeMode;

	const ResourceManager::Strings getButtonText() const;
	void toggleToolbar();
};
