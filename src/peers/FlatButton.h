#pragma once

/*
чтобы работало рисование, нужно в родительском окне добавлять
REFLECT_NOTIFICATIONS();
*/

class FlatButton: public CWindowImpl<FlatButton, CButton> {
public:
  enum TextAlignment {
    taLeft,
    taRight,
    taCenter
  };
private:
  bool m_allowFocus;
  UINT m_hintTime;
  tstring m_hintText;
  CToolTipCtrl m_hintControl;
  bool m_enabled;
  bool m_hover;
  int m_leftPadding;
  TextAlignment m_alignment;
  COLORREF m_textColor;
  COLORREF m_backgroundColor;
  COLORREF m_hiliteColor;
  bool m_highLightBorder;
  COLORREF m_highLightBorderColor;
  void init();
protected:
	virtual void drawBackrgound(CDCHandle dc, CRect &r);
	virtual void drawButton(CDCHandle dc, const CRect &r, bool selected);
	int getLeftPadding() const { return m_leftPadding; }
public:
  FlatButton();
  FlatButton(TextAlignment alignment);

  virtual void setAlignment(TextAlignment alignment) { m_alignment = alignment; }
  void setLeftPadding(int value) { m_leftPadding = value; }

  HWND Create(HWND hWndParent, LPCTSTR szWindowName, _U_MENUorID MenuOrID);
  HWND Create(HWND hWndParent, ATL::_U_RECT rect, LPCTSTR szWindowName, _U_MENUorID MenuOrID);

  BOOL IsWindowEnabled() const throw() { 
    return m_enabled;
  }

  BOOL EnableWindow(BOOL bEnable = TRUE) throw() {
    if ((bEnable != 0) != m_enabled) {
      return CWindowImpl::EnableWindow(bEnable);
    }
    else {
      return !m_enabled;
    }
  }

  bool isAllowFocus() const throw() {
    return m_allowFocus;
  }

  void setAllowFocus(bool value) throw() {
    m_allowFocus = value;
  }

  void setColors(COLORREF textColor, COLORREF backgroundColor, COLORREF hiliteColor) {
    m_textColor = textColor;
    m_backgroundColor = backgroundColor;
    m_hiliteColor = hiliteColor;
  }

  void setBackgroundColor(COLORREF backgroundColor) {
    m_backgroundColor = backgroundColor;
  }

  void setHighLightColor(COLORREF hiliteColor) {
    m_hiliteColor = hiliteColor;
  }

  void setHighLightBorder(bool value) {
    m_highLightBorder = value;
  }

  void setHintText(const tstring& hintText) {
    m_hintText = hintText;
  }

  bool isHover() const {
	  return m_hover;
  }

  virtual SIZE getPreferredSize();

  BEGIN_MSG_MAP(FlatButton)
    MESSAGE_HANDLER(WM_ENABLE, onEnable);
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(OCM_DRAWITEM, onDrawItem);
    MESSAGE_HANDLER(WM_MOUSEMOVE, onMouseMove);
    MESSAGE_HANDLER(WM_MOUSELEAVE, onMouseLeave);
    MESSAGE_HANDLER(WM_MOUSEHOVER, onMouseHover);
    MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown);
    MESSAGE_HANDLER(WM_LBUTTONUP, onLButtonUp);
  END_MSG_MAP();

  LRESULT onEnable(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT onLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
