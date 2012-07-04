/* 
* Copyright (C) 2003-2005 P�r Bj�rklund, per.bjorklund@gmail.com
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

#ifndef POPUPWND_H
#define POPUPWND_H

class PopupWnd : public CWindowImpl<PopupWnd, CWindow>
{
public:
	DECLARE_WND_CLASS(_T("Popup"));

	BEGIN_MSG_MAP(PopupWnd)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
	END_MSG_MAP()

	PopupWnd(const tstring& aMsg, const tstring& aTitle, CRect rc, uint32_t aId, HBITMAP hBmp): visible(GET_TICK()), id(aId), msg(aMsg), title(aTitle), bmp(hBmp) {
		if((SETTING(POPUP_TYPE) == BALLOON) || (SETTING(POPUP_TYPE) == SPLASH))
		Create(NULL, rc, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );
		else if((SETTING(POPUP_TYPE) == CUSTOM) && (bmp != NULL))
			Create(NULL, rc, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );
		else
			Create(NULL, rc, NULL, WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );

		WinUtil::decodeFont(Text::toT(SETTING(POPUP_FONT)), logFont);
		font = ::CreateFontIndirect(&logFont);

		WinUtil::decodeFont(Text::toT(SETTING(POPUP_TITLE_FONT)), myFont);
		titlefont = ::CreateFontIndirect(&myFont);

	}

	~PopupWnd(){
		DeleteObject(font);
		DeleteObject(titlefont);
	}

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		::PostMessage(WinUtil::mainWnd, WM_SPEAKER, WM_CLOSE, (LPARAM)id);
		bHandled = TRUE;
		return 0;
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		if(bmp != NULL && (SETTING(POPUP_TYPE) == CUSTOM)) {
			bHandled = FALSE;
			return 1;
		}

		::SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)::GetSysColorBrush(COLOR_INFOTEXT));
		CRect rc;
		GetClientRect(rc);

		rc.top += 1;
		rc.left += 1;
		rc.right -= 1;
		if((SETTING(POPUP_TYPE) == BALLOON) || (SETTING(POPUP_TYPE) == CUSTOM) || (SETTING(POPUP_TYPE) == SPLASH))
		rc.bottom /= 3;
		else
			rc.bottom /= 4;

		label.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			SS_CENTER | SS_NOPREFIX);

		rc.top += rc.bottom - 1;
		if((SETTING(POPUP_TYPE) == BALLOON) || (SETTING(POPUP_TYPE) == CUSTOM) || (SETTING(POPUP_TYPE) == SPLASH)) 
		rc.bottom *= 3;
		else
			rc.bottom = (rc.bottom * 4) + 1;

		label1.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			SS_CENTER | SS_NOPREFIX);

		if(SETTING(POPUP_TYPE) == BALLOON) {
		label.SetFont(WinUtil::boldFont);
			label.SetWindowText(title.c_str());
		label1.SetFont(WinUtil::font);
			label1.SetWindowText(msg.c_str());
			bHandled = false;
			return 1;
		} else if(SETTING(POPUP_TYPE) == CUSTOM || (SETTING(POPUP_TYPE) == SPLASH)) {
			label.SetFont(WinUtil::boldFont);
			label.SetWindowText(title.c_str());
		} else
			SetWindowText(title.c_str());

		label1.SetFont(font);
		label1.SetWindowText(msg.c_str());

		bHandled = false;
		return 1;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		if(bmp == NULL || (SETTING(POPUP_TYPE) != CUSTOM)){
		    DestroyAndDetachWindow(label);
		    DestroyAndDetachWindow(label1);
			}
		DestroyWindow();

		bHandled = false;
		return 1;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		::SetBkColor(hDC, SETTING(POPUP_BACKCOLOR));
		::SetTextColor(hDC, SETTING(POPUP_TEXTCOLOR));
		if(hWnd == label1.m_hWnd)
			::SelectObject(hDC, font);
		return (LRESULT)CreateSolidBrush(SETTING(POPUP_BACKCOLOR));
	}

	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if(bmp == NULL || (SETTING(POPUP_TYPE) != CUSTOM)){
			bHandled = FALSE;
			return 0;
		}

		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(m_hWnd,&ps);

		HDC hdcMem = CreateCompatibleDC(NULL);
		HBITMAP hbmT = (HBITMAP)::SelectObject(hdcMem,bmp);

		BITMAP bm;
		GetObject(bmp,sizeof(bm),&bm);

		//Move selected bitmap to the background
		BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);

		SelectObject(hdcMem,hbmT);
		DeleteDC(hdcMem);

		//Cofigure border
		::SetBkMode(hdc, TRANSPARENT);

		int xBorder = bm.bmWidth / 10;
		int yBorder = bm.bmHeight / 10;
		CRect rc(xBorder, yBorder, bm.bmWidth - xBorder, bm.bmHeight - yBorder);

		//Draw the Title and Message with selected font and color
		tstring pmsg = _T("\r\n\r\n") + msg;
		HFONT oldTitleFont = (HFONT)SelectObject(hdc, titlefont);
		::SetTextColor(hdc, SETTING(POPUP_TITLE_TEXTCOLOR));
		::DrawText(hdc, title.c_str(), title.length(), rc, DT_SINGLELINE | DT_TOP | DT_CENTER);

		HFONT oldFont = (HFONT)SelectObject(hdc, font);
		::SetTextColor(hdc, SETTING(POPUP_TEXTCOLOR));
		::DrawText(hdc, pmsg.c_str(), pmsg.length(), rc, DT_LEFT | DT_WORDBREAK);

		SelectObject(hdc, oldTitleFont);
		SelectObject(hdc, oldFont);
		::EndPaint(m_hWnd,&ps);

		return 0;
	}

	uint32_t id;
	uint64_t visible;
	uint16_t height;

	enum { BALLOON, CUSTOM, SPLASH, WINDOW };

private:
	tstring  msg, title;
	CStatic label, label1;
	LOGFONT logFont;
	HFONT   font;
	LOGFONT myFont;
	HFONT   titlefont;
	HBITMAP bmp;
};


#endif