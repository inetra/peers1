//
#if !defined(EXCEPTION_DLG_H)
#define EXCEPTION_DLG_H

class CExceptionDlg : public CDialogImpl<CExceptionDlg> {
private:
	const tstring m_exceptioninfo;
public:
	enum { IDD = IDD_EXCEPTION };
	CExceptionDlg(const tstring& exceptioninfo): m_exceptioninfo(exceptioninfo), m_hIcon(NULL) { }

	BEGIN_MSG_MAP(CExceptionDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_COPY_EXCEPTION, BN_CLICKED, OnCopyException)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnContinue)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnTerminate)
	END_MSG_MAP()

	void CopyEditToClipboard(HWND hWnd)	{
		SendMessage(hWnd, EM_SETSEL, 0, 65535L);
		SendMessage(hWnd, WM_COPY, 0 , 0);
		SendMessage(hWnd, EM_SETSEL, 0, 0);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		// Translate Dialog
		SetWindowText((_T(APPNAME) + TSTRING(EXCEPTION_DIALOG)).c_str());
		SetDlgItemText(IDC_EXCEPTION_HEADER, CTSTRING(EXCEPTION_HEADER));
		SetDlgItemText(IDC_EXCEPTION_FOOTER, CTSTRING(EXCEPTION_FOOTER));
		SetDlgItemText(IDC_EXCEPTION_RESTART, CTSTRING(EXCEPTION_RESTART));
		SetDlgItemText(IDC_COPY_EXCEPTION, CTSTRING(EXCEPTION_COPY));
		SetDlgItemText(IDOK, CTSTRING(EXCEPTION_CONTINUE));
		SetDlgItemText(IDCANCEL, CTSTRING(EXCEPTION_TERMINATE));

		m_hIcon.LoadIcon(IDR_STOP);
		SetIcon(m_hIcon, FALSE);
		SetIcon(m_hIcon, TRUE);

		CenterWindow();
		CEdit ctrlEI(GetDlgItem(IDC_EXCEPTION_DETAILS));
		ctrlEI.FmtLines(FALSE);
		ctrlEI.AppendText(m_exceptioninfo.c_str(), FALSE);

		return TRUE;
	}

	LRESULT OnCopyException(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CopyEditToClipboard(GetDlgItem(IDC_EXCEPTION_DETAILS));
		return 0;
	}

	LRESULT OnContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnTerminate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if (IsDlgButtonChecked(IDC_EXCEPTION_RESTART) == BST_CHECKED) {
			shutdown(true);
			_execl(WinUtil::getAppName().c_str(), WinUtil::getAppName().c_str(), "/c", NULL);
		}
		EndDialog(IDCANCEL);
		return 0;
	}

private:
	CIcon m_hIcon;
	CExceptionDlg(const CExceptionDlg&) { dcassert(0); };
};

#endif // !defined(EXCEPTION_DLG_H)
