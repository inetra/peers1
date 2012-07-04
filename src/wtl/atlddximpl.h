// Windows Template Library extension - classes to use with DDX_CONTROL.
// Written by Elijah Zarezky, copyright (c) 2006-2007.
//
// Purpose: "In WTL 7.0, if you wanted to hook up a plain window interface class
// (such as CWindow, CListViewCtrl, etc.) with DDX, you couldn't use DDX_CONTROL
// because DDX_CONTROL only works with CWindowImpl-derived classes." (c) Michael
// Dunn, visit http://www.codeproject.com/wtl/wtl4mfc4.asp for source
//
// The use and distribution terms for this software are covered by the
// Common Public License 1.0 (http://opensource.org/osi3.0/licenses/cpl1.0.php)
// which can be found in the file CPL.TXT at the root of this distribution.
// By using this software in any fashion, you are agreeing to be bound by
// the terms of this license. You must not remove this notice, or
// any other, from this software.

#if !defined(__ATLDDXIMPL_H__)
#define __ATLDDXIMPL_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#if !defined(_WTL_VER)
#include <atlapp.h>
#endif	// _WTL_VER

#if (_WTL_VER < 0x0710) || defined(_ATL_USE_DDX_IMPL)

///////////////////////////////////////////////////////////////////////////////
// Classes in this file:
//
// CStaticImpl
// CButtonImpl
// CListBoxImpl
// CComboBoxImpl
// CEditImpl
// CScrollBarImpl
//
// CToolTipCtrlImpl
// CHeaderCtrlImpl
// CListViewCtrlImpl
// CTreeViewCtrlImpl
// CTreeViewCtrlExImpl
// CToolBarCtrlImpl
// CStatusBarCtrlImpl
// CTabCtrlImpl
// CTrackBarCtrlImpl
// CUpDownCtrlImpl
// CProgressBarCtrlImpl
// CHotKeyCtrlImpl
// CAnimateCtrlImpl
// CRichEditCtrlImpl
// CDragListBoxImpl
// CReBarCtrlImpl
// CComboBoxExImpl
// CMonthCalendarCtrlImpl
// CDateTimePickerCtrlImpl
// CIPAddressCtrlImpl
// CPagerCtrlImpl
// CLinkCtrlImpl


namespace WTL
{

// CStaticImpl - ATL::CWindowImpl-derived class that implements a static.
class CStaticImpl : public ATL::CWindowImpl<CStaticImpl, CStatic>
{
public:
	BEGIN_MSG_MAP(CStaticImpl)
	END_MSG_MAP()
};


// CButtonImpl - ATL::CWindowImpl-derived class that implements a button.
class CButtonImpl : public ATL::CWindowImpl<CButtonImpl, CButton>
{
public:
	BEGIN_MSG_MAP(CButtonImpl)
	END_MSG_MAP()
};


// CListBoxImpl - ATL::CWindowImpl-derived class that implements a list-box.
class CListBoxImpl : public ATL::CWindowImpl<CListBoxImpl, CListBox>
{
public:
	BEGIN_MSG_MAP(CListBoxImpl)
	END_MSG_MAP()
};


#if !defined(WIN32_PLATFORM_WFSP)

// CComboBoxImpl - ATL::CWindowImpl-derived class that implements a combo-box.
class CComboBoxImpl : public ATL::CWindowImpl<CComboBoxImpl, CComboBox>
{
public:
	BEGIN_MSG_MAP(CComboBoxImpl)
	END_MSG_MAP()
};

#endif // WIN32_PLATFORM_WFSP


// CEditImpl - ATL::CWindowImpl-derived class that implements a edit.
class CEditImpl : public ATL::CWindowImpl<CEditImpl, CEdit>
{
public:
	BEGIN_MSG_MAP(CEditImpl)
	END_MSG_MAP()
};


// CScrollBarImpl - ATL::CWindowImpl-derived class that implements a scroll bar.
class CScrollBarImpl : public ATL::CWindowImpl<CScrollBarImpl, CScrollBar>
{
public:
	BEGIN_MSG_MAP(CScrollBarImpl)
	END_MSG_MAP()
};


#if !defined(_WIN32_WCE)

// CToolTipCtrlImpl - ATL::CWindowImpl-derived class that implements a tooltip.
class CToolTipCtrlImpl : public ATL::CWindowImpl<CToolTipCtrlImpl, CToolTipCtrl>
{
public:
	BEGIN_MSG_MAP(CToolTipCtrlImpl)
	END_MSG_MAP()
};

#endif // _WIN32_WCE


// CHeaderCtrlImpl - ATL::CWindowImpl-derived class that implements a header control.
class CHeaderCtrlImpl : public ATL::CWindowImpl<CHeaderCtrlImpl, CHeaderCtrl>
{
public:
	BEGIN_MSG_MAP(CHeaderCtrl)
	END_MSG_MAP()
};


// CListViewCtrlImpl - ATL::CWindowImpl-derived class that implements a list-view control.
class CListViewCtrlImpl : public ATL::CWindowImpl<CListViewCtrlImpl, CListViewCtrl>
{
public:
	BEGIN_MSG_MAP(CListViewCtrlImpl)
	END_MSG_MAP()
};


// CTreeViewCtrlImpl - ATL::CWindowImpl-derived class that implements a tree-view control.
class CTreeViewCtrlImpl : public ATL::CWindowImpl<CTreeViewCtrlImpl, CTreeViewCtrl>
{
public:
	BEGIN_MSG_MAP(CTreeViewCtrlImpl)
	END_MSG_MAP()
};


// CTreeViewCtrlExImpl - ATL::CWindowImpl-derived class that implements a extended tree-view control.
class CTreeViewCtrlExImpl : public ATL::CWindowImpl<CTreeViewCtrlExImpl, CTreeViewCtrlEx>
{
public:
	BEGIN_MSG_MAP(CTreeViewCtrlExImpl)
	END_MSG_MAP()
};


// CToolBarCtrlImpl - ATL::CWindowImpl-derived class that implements a toolbar.
class CToolBarCtrlImpl : public ATL::CWindowImpl<CToolBarCtrlImpl, CToolBarCtrl>
{
public:
	BEGIN_MSG_MAP(CToolBarCtrlImpl)
	END_MSG_MAP()
};


// CStatusBarCtrlImpl - ATL::CWindowImpl-derived class that implements a status bar.
class CStatusBarCtrlImpl : public ATL::CWindowImpl<CStatusBarCtrlImpl, CStatusBarCtrl>
{
public:
	BEGIN_MSG_MAP(CStatusBarCtrlImpl)
	END_MSG_MAP()
};


// CTabCtrlImpl - ATL::CWindowImpl-derived class that implements a tab control.
class CTabCtrlImpl : public ATL::CWindowImpl<CTabCtrlImpl, CTabCtrl>
{
public:
	BEGIN_MSG_MAP(CTabCtrlImpl)
	END_MSG_MAP()
};


// CTrackBarCtrlImpl - ATL::CWindowImpl-derived class that implements a slider.
class CTrackBarCtrlImpl : public ATL::CWindowImpl<CTrackBarCtrlImpl, CTrackBarCtrl>
{
public:
	BEGIN_MSG_MAP(CTrackBarCtrlImpl)
	END_MSG_MAP()
};


// CUpDownCtrlImpl - ATL::CWindowImpl-derived class that implements a up-down control.
class CUpDownCtrlImpl : public ATL::CWindowImpl<CUpDownCtrlImpl, CUpDownCtrl>
{
public:
	BEGIN_MSG_MAP(CUpDownCtrlImpl)
	END_MSG_MAP()
};


// CProgressBarCtrlImpl - ATL::CWindowImpl-derived class that implements a progress bar.
class CProgressBarCtrlImpl : public ATL::CWindowImpl<CProgressBarCtrlImpl, CProgressBarCtrl>
{
public:
	BEGIN_MSG_MAP(CProgressBarCtrlImpl)
	END_MSG_MAP()
};


#if !defined(_WIN32_WCE)

// CHotKeyCtrlImpl - ATL::CWindowImpl-derived class that implements a hot-key control.
class CHotKeyCtrlImpl : public ATL::CWindowImpl<CHotKeyCtrlImpl, CHotKeyCtrl>
{
public:
	BEGIN_MSG_MAP(CHotKeyCtrlImpl)
	END_MSG_MAP()
};


// CAnimateCtrlImpl - ATL::CWindowImpl-derived class that implements a "animate" control.
class CAnimateCtrlImpl : public ATL::CWindowImpl<CAnimateCtrlImpl, CAnimateCtrl>
{
public:
	BEGIN_MSG_MAP(CAnimateCtrlImpl)
	END_MSG_MAP()
};


// CRichEditCtrlImpl - ATL::CWindowImpl-derived class that implements a rich-edit control.
class CRichEditCtrlImpl : public ATL::CWindowImpl<CRichEditCtrlImpl, CRichEditCtrl>
{
public:
	BEGIN_MSG_MAP(CRichEditCtrlImpl)
	END_MSG_MAP()
};


// CDragListBoxImpl - ATL::CWindowImpl-derived class that implements a drag list-box.
class CDragListBoxImpl : public ATL::CWindowImpl<CDragListBoxImpl, CDragListBox>
{
public:
	BEGIN_MSG_MAP(CDragListBoxImpl)
	END_MSG_MAP()
};

#endif // _WIN32_WCE


// CReBarCtrlImpl - ATL::CWindowImpl-derived class that implements a re-bar control.
class CReBarCtrlImpl : public ATL::CWindowImpl<CReBarCtrlImpl, CReBarCtrl>
{
public:
	BEGIN_MSG_MAP(CReBarCtrlImpl)
	END_MSG_MAP()
};


#if !defined(_WIN32_WCE)

// CComboBoxExImpl - ATL::CWindowImpl-derived class that implements a extended combo-box.
class CComboBoxExImpl : public ATL::CWindowImpl<CComboBoxExImpl, CComboBoxEx>
{
public:
	BEGIN_MSG_MAP(CComboBoxExImpl)
	END_MSG_MAP()
};

#endif // _WIN32_WCE


// CMonthCalendarCtrlImpl - ATL::CWindowImpl-derived class that implements a calendar control.
class CMonthCalendarCtrlImpl : public ATL::CWindowImpl<CMonthCalendarCtrlImpl, CMonthCalendarCtrl>
{
public:
	BEGIN_MSG_MAP(CMonthCalendarCtrlImpl)
	END_MSG_MAP()
};


// CDateTimePickerCtrlImpl - ATL::CWindowImpl-derived class that implements a date-time picker.
class CDateTimePickerCtrlImpl : public ATL::CWindowImpl<CDateTimePickerCtrlImpl, CDateTimePickerCtrl>
{
public:
	BEGIN_MSG_MAP(CDateTimePickerCtrlImpl)
	END_MSG_MAP()
};


#if (_WIN32_IE >= 0x0400)

// CIPAddressCtrlImpl - ATL::CWindowImpl-derived class that implements a IP-address control.
class CIPAddressCtrlImpl : public ATL::CWindowImpl<CIPAddressCtrlImpl, CIPAddressCtrl>
{
public:
	BEGIN_MSG_MAP(CIPAddressCtrlImpl)
	END_MSG_MAP()
};

#endif // (_WIN32_IE >= 0x0400)


#if (_WIN32_IE >= 0x0400) && !defined(_WIN32_WCE)

// CPagerCtrlImpl - ATL::CWindowImpl-derived class that implements a pager control.
class CPagerCtrlImpl : public ATL::CWindowImpl<CPagerCtrlImpl, CPagerCtrl>
{
public:
	BEGIN_MSG_MAP(CPagerCtrlImpl)
	END_MSG_MAP()
};

#endif // (_WIN32_IE >= 0x0400) && !defined(_WIN32_WCE)


#if (_WIN32_WINNT >= 0x0501) && !defined(_WIN32_WCE)

// CLinkCtrlImpl - ATL::CWindowImpl-derived class that implements a hyperlink control.
class CLinkCtrlImpl : public ATL::CWindowImpl<CLinkCtrlImpl, CLinkCtrl>
{
public:
	BEGIN_MSG_MAP(CLinkCtrlImpl)
	END_MSG_MAP()
};

#endif // (_WIN32_WINNT >= 0x0501) && !defined(_WIN32_WCE)

}; // namespace WTL

#endif // _WTL_VER || _ATL_USE_DDX_IMPL

#endif // __ATLDDXIMPL_H__

// end of file
