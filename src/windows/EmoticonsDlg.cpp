#include "stdafx.h"
#include "EmoticonsDlg.h"
#ifndef AGEMOTIONSETUP_H__
#include "AGEmotionSetup.h"
#endif

#define EMOTICONS_ICONMARGIN 8

extern CAGEmotionSetup* g_pEmotionsSetup;

WNDPROC EmoticonsDlg::m_MFCWndProc = 0;
EmoticonsDlg* EmoticonsDlg::m_pDialog = NULL;

LRESULT EmoticonsDlg::onEmoticonClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	::GetWindowText(hWndCtl, buf, 255);
	result = buf;
	// pro ucely testovani emoticon packu...
	if ((GetKeyState(VK_SHIFT) & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) {
		const CAGEmotion::List& Emoticons = g_pEmotionsSetup->getEmoticonsList();
		result = _T("");
		string lastEmotionPath;
		int l_count_emotion = 0;
		for(CAGEmotion::Iter pEmotion = Emoticons.begin(); 
			pEmotion != Emoticons.end() && l_count_emotion < g_pEmotionsSetup->m_CountSelEmotions;
			++pEmotion,++l_count_emotion) 
		{
			if (lastEmotionPath != (*pEmotion)->getEmotionBmpPath()) 
				  result += (*pEmotion)->getEmotionText() + _T(" ");
			lastEmotionPath = (*pEmotion)->getEmotionBmpPath();
		}
	}
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT EmoticonsDlg::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	WNDPROC temp = reinterpret_cast<WNDPROC>(::SetWindowLong(EmoticonsDlg::m_hWnd, GWL_WNDPROC, reinterpret_cast<LONG>(NewWndProc)));
	if (!m_MFCWndProc) m_MFCWndProc = temp;
	m_pDialog = this;
	::EnableWindow(WinUtil::mainWnd, true);

	if(g_pEmotionsSetup->getUseEmoticons() && SETTING(EMOTICONS_FILE)!="Disabled") {

		const CAGEmotion::List& Emoticons = g_pEmotionsSetup->getEmoticonsList();

		unsigned int pocet = 0;
		int l_count_emotion = 0;
		string lastEmotionPath;
		CAGEmotion::Iter pEmotion;
		for(pEmotion = Emoticons.begin(); 
			pEmotion != Emoticons.end() && l_count_emotion < g_pEmotionsSetup->m_CountSelEmotions;
			++pEmotion,++l_count_emotion)
		{
			if ((*pEmotion)->getEmotionBmpPath() != lastEmotionPath)
				pocet++;
			lastEmotionPath = (*pEmotion)->getEmotionBmpPath();
		}

		// x, y jen pro for cyklus
		const unsigned int l_Emoticons_size = g_pEmotionsSetup->m_CountSelEmotions;
		unsigned int i = (unsigned int)sqrt(double(l_Emoticons_size));
		unsigned int nXfor = i;
		unsigned int nYfor = i;
		
		if ((i*i) != l_Emoticons_size) //[+]PPA
		{
			nXfor = i+1;
			if ((i*nXfor) < l_Emoticons_size) nYfor = i+1;
			else nYfor = i;
		}
		// x, y pro korektni vkladani ikonek za sebou
		i = (unsigned int)sqrt((double)pocet);
		unsigned int nX = i;
		unsigned int nY = i;
		if ((i*i) != pocet)  //[+]PPA
		{
			nX = i+1;
			if ((i*nX) < pocet) nY = i+1;
			else nY = i;
		}
        if (!*Emoticons.begin()) //[+]PPA
			return 0; 

		HBITMAP h_bm = (*Emoticons.begin())->getEmotionBmp(GetSysColor(COLOR_BTNFACE));
		BITMAP bm;
		GetObject(h_bm, sizeof(BITMAP), &bm);
		DeleteObject(h_bm);
		int bW = bm.bmWidth;

		pos.bottom = pos.top - 3;
		pos.left = pos.right - nX * (bW + EMOTICONS_ICONMARGIN) - 2;
		pos.top = pos.bottom - nY * (bW + EMOTICONS_ICONMARGIN) - 2;
		EmoticonsDlg::MoveWindow(pos);

		pEmotion = Emoticons.begin();
		lastEmotionPath = "";

		ctrlToolTip.Create(EmoticonsDlg::m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
		ctrlToolTip.SetDelayTime(TTDT_AUTOMATIC, 1000);

		pos.left = 0;
		pos.right = bW + EMOTICONS_ICONMARGIN;
		pos.top = 0;
		pos.bottom = bW + EMOTICONS_ICONMARGIN;

		HandleList.clear();
		for (unsigned int iY = 0; iY < nYfor; iY++)
		for (unsigned int iX = 0; iX < nXfor; iX++)
		{
			if ((iY*nXfor)+iX+1 > l_Emoticons_size) break;

			// dve stejne emotikony za sebou nechceme
			if ((*pEmotion)->getEmotionBmpPath() != lastEmotionPath) {
				try {
					CButton emoButton;
					emoButton.Create(EmoticonsDlg::m_hWnd, pos, (*pEmotion)->getEmotionText().c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER);
					HBITMAP h_bm = (*pEmotion)->getEmotionBmp(GetSysColor(COLOR_BTNFACE));
					HandleList.push_back(h_bm);
					emoButton.SetBitmap(h_bm);

					tstring smajl = (*pEmotion)->getEmotionText();
					CToolInfo ti(TTF_SUBCLASS, emoButton.m_hWnd, 0, 0, const_cast<LPTSTR>(smajl.c_str()));
					ctrlToolTip.AddTool(&ti);

					DeleteObject((HGDIOBJ)emoButton);
				} catch (...) { }

				pos.left = pos.left + bW + EMOTICONS_ICONMARGIN;
				pos.right = pos.left + bW + EMOTICONS_ICONMARGIN;
				if (pos.left >= (LONG)(nX*(bW+EMOTICONS_ICONMARGIN))) {
					pos.left = 0;
					pos.right = bW + EMOTICONS_ICONMARGIN;
					pos.top = pos.top + bW + EMOTICONS_ICONMARGIN;
					pos.bottom = pos.top + bW + EMOTICONS_ICONMARGIN;
				}
			}

			lastEmotionPath = (*pEmotion)->getEmotionBmpPath();
			try { pEmotion++; }
			catch (...) {}
		}

		try {
			pos.left = -128;
			pos.right = pos.left;
			pos.top = -128;
			pos.bottom = pos.top;
			CButton emoButton;
			emoButton.Create(EmoticonsDlg::m_hWnd, pos, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT);
			emoButton.SetFocus();
			DeleteObject((HGDIOBJ)emoButton);
		} catch (...) { }

	} else PostMessage(WM_CLOSE);

	return 0;
}

LRESULT CALLBACK EmoticonsDlg::NewWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message==WM_ACTIVATE && wParam==0) {
		m_pDialog->PostMessage(WM_CLOSE);
		return FALSE;
    }
	return ::CallWindowProc(m_MFCWndProc, hWnd, message, wParam, lParam);
}
