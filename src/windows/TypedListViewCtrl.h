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

#if !defined(TYPED_LIST_VIEW_CTRL_H)
#define TYPED_LIST_VIEW_CTRL_H

#include "../client/FavoriteManager.h"
#include "ListViewArrows.h"

class ColumnInfo {
public:
  ColumnInfo(const tstring &aName, int aPos, int aFormat, int aWidth, int subItem): m_name(aName), m_pos(aPos), m_width(aWidth), m_format(aFormat), m_subItem(subItem), m_visible(true) {
  }
  ~ColumnInfo() {}
  bool m_visible;
  unsigned m_width;
  unsigned m_format;
  int m_pos;
  int m_subItem;
  tstring m_name;
};

struct ListColumn {
  
  enum AlignmentEnum {
    LEFT,
    RIGHT,
    CENTER
  };

  class Alignment {
  private:
    int m_format;
  public:
    Alignment(): m_format(LVCFMT_LEFT) {
    }
    Alignment(int format): m_format(format) {
    }
    Alignment(AlignmentEnum e): m_format(e == LEFT ? LVCFMT_LEFT : (e == RIGHT ? LVCFMT_RIGHT : LVCFMT_CENTER)) {
    }
    operator int () const { return m_format; }
  };

  int index;
  int size;
  ResourceManager::Strings name;
  bool visible;
  Alignment alignment;
};

struct ListColumnSettings {
  SettingsManager::StrSetting columnOrder;
  SettingsManager::StrSetting columnWidths;
  SettingsManager::StrSetting columnVisibility;
};

template<class T, int ctrlId>
class TypedListViewCtrl : public CWindowImpl<TypedListViewCtrl<T, ctrlId>, CListViewCtrl, CControlWinTraits>,
	public ListViewArrows<TypedListViewCtrl<T, ctrlId> > 
{
public:
	TypedListViewCtrl() : sortColumn(-1), sortAscending(true), hBrBg(WinUtil::bgBrush), leftMargin(0) 
	{ 
//[?]PPA	columnList.reserve(6);
//[?]PPA        columnIndexes.reserve(6);
	}
	~TypedListViewCtrl() { for_each(columnList.begin(), columnList.end(), DeleteFunction()); }

	typedef TypedListViewCtrl<T, ctrlId> thisClass;
	typedef CListViewCtrl baseClass;
	typedef ListViewArrows<thisClass> arrowBase;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_MENUCOMMAND, onHeaderMenu)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		CHAIN_MSG_MAP(arrowBase)
	END_MSG_MAP();

        class Selection {
        private:
          TypedListViewCtrl& m_owner;
          int m_index;
        public:
          Selection(TypedListViewCtrl& owner): m_owner(owner) { 
            m_index = owner.GetNextItem(-1, LVNI_SELECTED);
          }
          Selection(const Selection& source): m_owner(source.m_owner), m_index(source.m_index) {
          }
          bool hasNext() const {
            return m_index != -1;
          }
          T* next() {
            dcassert(m_index >= 0);
            T *item = m_owner.getItemData(m_index);
            m_index = m_owner.GetNextItem(m_index, LVNI_SELECTED);
            return item;
          }
        };

        template <size_t size> void InitColumns(const ListColumn (&columns)[size], const ListColumnSettings& listSettings) {
          ListColumn tempColumns[size];
          memcpy(tempColumns, columns, sizeof(ListColumn) * size);
          int temp[size];
          // load widths
          for (int i = 0; i < size; ++i) temp[i] = columns[i].size;
          WinUtil::splitTokens(temp, SettingsManager::getInstance()->get(listSettings.columnWidths), size);
          for (int i = 0; i < size; ++i) tempColumns[i].size = temp[i];
          // create columns
          for (int i = 0; i < size; ++i) {
            InsertColumn(i, 
              CTSTRING_I(tempColumns[i].name), 
              tempColumns[i].alignment, 
              tempColumns[i].size,
              tempColumns[i].index);
          }
          // load order
          for (int i = 0; i < size; ++i) temp[i] = i;
          WinUtil::splitTokens(temp, SettingsManager::getInstance()->get(listSettings.columnOrder), size);
          setColumnOrderArray(size, temp);
          // load visibility
          for (int i = 0; i < size; ++i) temp[i] = columns[i].visible;
          WinUtil::splitTokens(temp, SettingsManager::getInstance()->get(listSettings.columnVisibility), size);
          for (int i = 0; i < size; ++i) {
            if (!temp[i]) {
              columnList[i]->m_visible = false;
              removeColumn(columnList[i]);
            }
          }
          updateColumnIndexes();
        }

        template <size_t size> void InsertColumns(const ListColumn (&columns)[size]) {
          for (int i = 0; i < size; ++i) {
            InsertColumn(i, 
              CTSTRING_I(columns[i].name), 
              columns[i].alignment, 
              columns[i].size,
              columns[i].index);
          }
          for (int i = 0; i < size; ++i) {
            if (!columns[i].visible) {
              columnList[i]->m_visible = false;
              removeColumn(columnList[i]);
            }
          }
          updateColumnIndexes();
        }

	class iterator : public ::iterator<random_access_iterator_tag, T*> {
	public:
		iterator() : typedList(NULL), cur(0), cnt(0) { }
		iterator(const iterator& rhs) : typedList(rhs.typedList), cur(rhs.cur), cnt(rhs.cnt) { }
		iterator& operator=(const iterator& rhs) { typedList = rhs.typedList; cur = rhs.cur; cnt = rhs.cnt; return *this; }

		bool operator==(const iterator& rhs) const { return cur == rhs.cur; }
		bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
		bool operator<(const iterator& rhs) const { return cur < rhs.cur; }

		int operator-(const iterator& rhs) const { 
			return cur - rhs.cur;
		}

		iterator& operator+=(int n) { cur += n; return *this; }
		iterator& operator-=(int n) { return (cur += -n); }
		
		T& operator*() { return *typedList->getItemData(cur); }
		T* operator->() { return &(*(*this)); }
		T& operator[](int n) { return *typedList->getItemData(cur + n); }
		
		iterator operator++(int) {
			iterator tmp(*this);
			operator++();
			return tmp;
		}
		iterator& operator++() {
			++cur;
			return *this;
		}

	private:
		iterator(thisClass* aTypedList) : typedList(aTypedList), cur(aTypedList->GetNextItem(-1, LVNI_ALL)), cnt(aTypedList->GetItemCount()) { 
			if(cur == -1)
				cur = cnt;
		}
		iterator(thisClass* aTypedList, int first) : typedList(aTypedList), cur(first), cnt(aTypedList->GetItemCount()) { 
			if(cur == -1)
				cur = cnt;
		}
		friend class thisClass;
		thisClass* typedList;
		int cur;
		int cnt;
	};

	LRESULT onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if((GetKeyState(VkKeyScan('A') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0){
			int count = GetItemCount();
			for(int i = 0; i < count; ++i)
				ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);

			return 0;
		}
		
		bHandled = FALSE;
		return 1;
	}

	LRESULT onGetDispInfo(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMLVDISPINFO* di = (NMLVDISPINFO*)pnmh;
		if(di->item.mask & LVIF_TEXT) {
			di->item.mask |= LVIF_DI_SETITEM;
			di->item.pszText = const_cast<TCHAR*>(((T*)di->item.lParam)->getText(columnIndexes[di->item.iSubItem]).c_str());
		}
		return 0;
	}
	
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		if(!BOOLSETTING(SHOW_INFOTIPS)) return 0;

		NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*) pnmh;
		BOOL NoColumnHeader = (BOOL)(GetWindowLong(GWL_STYLE) & LVS_NOCOLUMNHEADER);
		tstring InfoTip(Util::emptyStringT);
		AutoArray<TCHAR> Buffer(300);

		LV_COLUMN lvCol;
		int indexes[32];
		GetColumnOrderArray(GetHeader().GetItemCount(), indexes);
		for (int i = 0; i < GetHeader().GetItemCount(); ++i)
		{
			if (!NoColumnHeader) {
				lvCol.mask = LVCF_TEXT;
				lvCol.pszText = Buffer;
				lvCol.cchTextMax = 300;
				GetColumn(indexes[i], &lvCol);
				InfoTip += lvCol.pszText;
				InfoTip += _T(": ");
			}
			GetItemText(pInfoTip->iItem, indexes[i], Buffer, 300);
			Buffer[299] = NULL;

			InfoTip += Buffer;
			InfoTip += _T("\r\n");
		}

		if (InfoTip.size() > 2)
			InfoTip.erase(InfoTip.size() - 2);
		
		pInfoTip->cchTextMax = InfoTip.size();

 		_tcsncpy(pInfoTip->pszText, InfoTip.c_str(), INFOTIPSIZE);
		pInfoTip->pszText[INFOTIPSIZE - 1] = NULL;

		return 0;
	}

	// Sorting
	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem != sortColumn) {
			sortAscending = true;
			sortColumn = l->iSubItem;
		} else if(sortAscending) {
			sortAscending = false;
		} else {
			sortColumn = -1;
		}
		updateArrow();
		resort();
		return 0;
	}
	void resort() {
		if(sortColumn != -1) {
			SortItems(&compareFunc, (LPARAM)this);
		}
	}

	int insertItem(T* item, int image) {
		return insertItem(getSortPos(item), item, image);
	}
	int insertItem(int i, T* item, int image) {
		return InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i, 
			LPSTR_TEXTCALLBACK, 0, 0, image, (LPARAM)item);
	}
	T* getItemData(int iItem) const { return (T*)GetItemData(iItem); }
	T* getSelectedItem() { return (GetSelectedCount() > 0 ? getItemData(GetNextItem(-1, LVNI_SELECTED)) : NULL); }

	int findItem(T* item) { 
		LVFINDINFO fi = { LVFI_PARAM, NULL, (LPARAM)item };
		return FindItem(&fi, -1);
	}
#if 0
	struct CompFirst {
		CompFirst() { } 
		bool operator()(T& a, const tstring& b) {
			return Util::stricmp(a.getText(0), b) < 0;
		}
	};
#endif
	int findItem(const tstring& b, int start = -1, bool aPartial = false) {
		LVFINDINFO fi = { aPartial ? LVFI_PARTIAL : LVFI_STRING, b.c_str() };
		return FindItem(&fi, start);
	}

        int getSelectedCount() { return GetSelectedCount(); } // !SMT!-S
        void forEachSelectedParam(void (T::*func)(void*), void *param) { // !SMT!-S
                int i = -1;
                while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
                        (getItemData(i)->*func)(param);
        }

	void forEach(void (T::*func)()) {
		int n = GetItemCount();
		for(int i = 0; i < n; ++i)
			(getItemData(i)->*func)();
	}
	void forEachSelected(void (T::*func)()) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			(getItemData(i)->*func)();
	}
	template<class _Function>
	_Function forEachT(_Function pred) {
		int n = GetItemCount();
		for(int i = 0; i < n; ++i)
			pred(getItemData(i));
		return pred;
	}
	template<class _Function>
	_Function forEachSelectedT(_Function pred) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			pred(getItemData(i));
		return pred;
	}
	void forEachAtPos(int iIndex, void (T::*func)()) {
		(getItemData(iIndex)->*func)();
	}

	void updateItem(int i) {
		int k = GetHeader().GetItemCount();
		for(int j = 0; j < k; ++j)
			SetItemText(i, j, LPSTR_TEXTCALLBACK);
	}
	void updateItem(T* item) { int i = findItem(item); if(i != -1) updateItem(i); }
	void deleteItem(T* item) { int i = findItem(item); if(i != -1) DeleteItem(i); }

	int getSortPos(T* a) {
		int high = GetItemCount();
		if((sortColumn == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		T* b = NULL;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = getItemData(mid);
			comp = T::compareItems(a, b, sortColumn);
			
			if(!sortAscending)
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
					low = mid + 1;
			}
		}

		comp = T::compareItems(a, b, sortColumn);
		if(!sortAscending)
			comp = -comp;
		if(comp > 0)
			mid++;

		return mid;
	}

	void setSortColumn(int aSortColumn) {
		sortColumn = aSortColumn;
		updateArrow();
	}
	int getSortColumn() const { return sortColumn; }
	int getRealSortColumn() const { return findColumn(sortColumn); }
	bool isAscending() const { return sortAscending; }
	void setAscending(bool s) {
		sortAscending = s;
		updateArrow();
	}

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, GetItemCount()); }

	int InsertColumn(int nCol, const tstring &columnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1 ){
		if(nWidth == 0) 
		   nWidth = 80;
		columnList.push_back(new ColumnInfo(columnHeading, nCol, nFormat, nWidth, nSubItem));
		columnIndexes.push_back(uint8_t(nCol));
		return CListViewCtrl::InsertColumn(nCol, columnHeading.c_str(), nFormat, nWidth, nSubItem);
	}

	void showMenu(POINT &pt){
		headerMenu.DestroyMenu();
		headerMenu.CreatePopupMenu();
		MENUINFO inf;
		inf.cbSize = sizeof(MENUINFO);
		inf.fMask = MIM_STYLE;
		inf.dwStyle = MNS_NOTIFYBYPOS;
		headerMenu.SetMenuInfo(&inf);

		int j = 0;
		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i, ++j) {
			headerMenu.AppendMenu(MF_STRING, IDC_HEADER_MENU, (*i)->m_name.c_str());
			if((*i)->m_visible)
				headerMenu.CheckMenuItem(j, MF_BYPOSITION | MF_CHECKED);
		}
		headerMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	}
		
	LRESULT onEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		if(!leftMargin || !hBrBg) 
			return 0;

		dcassert(hBrBg);
		if(!hBrBg) return 0;

		bHandled = TRUE;
		HDC dc = (HDC)wParam;
		int n = GetItemCount();
		RECT r = {0, 0, 0, 0}, full;
		GetClientRect(&full);

		if (n > 0) {
			GetItemRect(0, &r, LVIR_BOUNDS);
			r.bottom = r.top + ((r.bottom - r.top) * n);
		}

		RECT full2 = full; // Keep a backup


		full.bottom = r.top;
		FillRect(dc, &full, hBrBg);

		full = full2; // Revert from backup
		full.right = r.left + leftMargin; // state image
		//full.left = 0;
		FillRect(dc, &full, hBrBg);

		full = full2; // Revert from backup
		full.left = r.right;
		FillRect(dc, &full, hBrBg);

		full = full2; // Revert from backup
		full.top = r.bottom;
		full.right = r.right;
		FillRect(dc, &full, hBrBg);

		
		return S_OK;
	}
	void setFlickerFree(HBRUSH flickerBrush) { hBrBg = flickerBrush; }

	LRESULT onContextMenu(UINT /*msg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		//make sure we're not handling the context menu if it's invoked from the
		//keyboard
		if(pt.x == -1 && pt.y == -1) {
			bHandled = FALSE;
			return 0;
		}

		CRect rc;
		GetHeader().GetWindowRect(&rc);

		if (PtInRect(&rc, pt)) {
			showMenu(pt);
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}
	
	LRESULT onHeaderMenu(UINT /*msg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ColumnInfo * ci = columnList[wParam];
		ci->m_visible = ! ci->m_visible;

		SetRedraw(FALSE);

		if(!ci->m_visible){
			removeColumn(ci);
		} else {
			if(ci->m_width == 0) 
			   ci->m_width = 80;
			CListViewCtrl::InsertColumn(ci->m_pos, ci->m_name.c_str(),
				0, //ci->format, 
				ci->m_width, static_cast<int>(wParam));
			LVCOLUMN lvcl = { 0 };
			lvcl.mask = LVCF_ORDER;
			lvcl.iOrder = ci->m_pos;
			SetColumn(ci->m_pos, &lvcl);
			for(int i = 0; i < GetItemCount(); ++i) {
				LVITEM lvItem;
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_IMAGE | LVIF_PARAM;
				GetItem(&lvItem);
				lvItem.iImage = ((T*)lvItem.lParam)->imageIndex();
				SetItem(&lvItem);
				updateItem(i);
			}
		}

		updateColumnIndexes();

		SetRedraw();
		Invalidate();
		UpdateWindow();

		return 0;
	}

//	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled */ ) {
/*

		    bHandled = FALSE;
			return 0;
		if(!BOOLSETTING(USE_CUSTOM_LIST_BACKGROUND)) {
			bHandled = FALSE;
			return 0;
		}

		LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

		switch(cd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT: {
//[-]PPA TODO			if(BOOLSETTING(USE_CUSTOM_LIST_BACKGROUND)) 
{
				if(cd->nmcd.dwItemSpec % 2 != 0) {
					cd->clrTextBk = HLS_TRANSFORM(((cd->clrTextBk == CLR_DEFAULT) ? ::GetSysColor(COLOR_WINDOW) : cd->clrTextBk), -9, 0);
				}
			}
			return CDRF_DODEFAULT;
		}

		default:
			return CDRF_DODEFAULT;
		}
*/
//	}

        void saveHeaderOrder(const ListColumnSettings& listSettings) {
          saveHeaderOrder(listSettings.columnOrder, listSettings.columnWidths, listSettings.columnVisibility);
        }

        void saveHeaderOrder(SettingsManager::StrSetting order, SettingsManager::StrSetting widths,
		SettingsManager::StrSetting visible) {
		string tmp, tmp2, tmp3;
                        saveHeaderOrder(tmp, tmp2, tmp3);
		SettingsManager::getInstance()->set(order, tmp);
		SettingsManager::getInstance()->set(widths, tmp2);
		SettingsManager::getInstance()->set(visible, tmp3);
	}

        void saveHeaderOrder(string& order, string& widths, string& visible) throw() {
		TCHAR buf[512];
		int size = GetHeader().GetItemCount();
		for(int i = 0; i < size; ++i){
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
			lvc.cchTextMax = 512;
			lvc.pszText = buf;
			GetColumn(i, &lvc);
			for(ColumnIter j = columnList.begin(); j != columnList.end(); ++j){
				if(_tcscmp(buf, (*j)->m_name.c_str()) == 0){
					(*j)->m_pos = lvc.iOrder;
					(*j)->m_width = lvc.cx;
					break;
				}
			}
		}

		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i){
			ColumnInfo* ci = *i;

			if(ci->m_visible){
				visible += "1,";
			} else {
				ci->m_pos = size++;
				visible += "0,";
			}

			order += Util::toString(ci->m_pos);
			order += ',';

			widths += Util::toString(ci->m_width);
			widths += ',';
		}

		order.erase(order.size()-1, 1);
		widths.erase(widths.size()-1, 1);
		visible.erase(visible.size()-1, 1);

	}
	
	void setVisible(const string& vis) {
		StringTokenizer<string> tok(vis, ',');
		StringList l = tok.getTokens();

		StringIter i = l.begin();
		for(ColumnIter j = columnList.begin(); j != columnList.end() && i != l.end(); ++i, ++j) {

			if(Util::toInt(*i) == 0){
				(*j)->m_visible = false;
				removeColumn(*j);
			}
		}

		updateColumnIndexes();
	}

        void setColumnOrderArray(int iCount, LPINT piArray ) {
                LVCOLUMN lvc;
                lvc.mask = LVCF_ORDER;
                for(int i = 0; i < iCount; ++i) {
                        lvc.iOrder = columnList[i]->m_pos = piArray[i];
                        SetColumn(i, &lvc);
                }
        }

	//find the original position of the column at the current position.
	inline uint8_t findColumn(int col) const { return columnIndexes[col]; }	
	
private:
        TypedListViewCtrl(const TypedListViewCtrl&) { dcassert(false); /* no copy constructor */ }
	int sortColumn;
	bool sortAscending;
	int leftMargin;
	HBRUSH hBrBg;
	CMenu headerMenu;	

	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		int result = T::compareItems((T*)lParam1, (T*)lParam2, t->getRealSortColumn());
		return (t->sortAscending ? result : -result);
	}

	typedef vector< ColumnInfo* > ColumnList;
	typedef ColumnList::const_iterator ColumnIter;

	ColumnList columnList;
	vector<uint8_t> columnIndexes;

	void removeColumn(ColumnInfo* ci){
		TCHAR buf[512];
		LVCOLUMN lvcl = { 0 };
		lvcl.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
		lvcl.pszText = buf;
		lvcl.cchTextMax = 512;

		for(int k = 0; k < GetHeader().GetItemCount(); ++k){
			GetColumn(k, &lvcl);
			if(_tcscmp(ci->m_name.c_str(), lvcl.pszText) == 0){
				ci->m_width = lvcl.cx;
				ci->m_pos = lvcl.iOrder;

				int itemCount = GetHeader().GetItemCount();
				if(itemCount >= 0 && sortColumn > itemCount - 2)
					setSortColumn(0);

				if(sortColumn == ci->m_pos)
					setSortColumn(0);
	
				DeleteColumn(k);

				for(int i = 0; i < GetItemCount(); ++i) {
					LVITEM lvItem;
					lvItem.iItem = i;
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_PARAM | LVIF_IMAGE;
					GetItem(&lvItem);
					lvItem.iImage = ((T*)lvItem.lParam)->imageIndex();
					SetItem(&lvItem);
				}
				break;
			}
		}
	}

	void updateColumnIndexes() {
		columnIndexes.clear();
		
		int columns = GetHeader().GetItemCount();
		
		columnIndexes.reserve(columns);

		TCHAR buf[128];
		LVCOLUMN lvcl;

		for(int i = 0; i < columns; ++i) {
			lvcl.mask = LVCF_TEXT;
			lvcl.pszText = buf;
			lvcl.cchTextMax = 128;
			GetColumn(i, &lvcl);
			for(uint8_t j = 0; j < columnList.size(); ++j) {
				if(Util::stricmp(columnList[j]->m_name.c_str(), lvcl.pszText) == 0) {
					columnIndexes.push_back(static_cast<uint8_t>(columnList[j]->m_subItem));
					break;
				}
			}
		}
	}	
};

// Copyright (C) 2005 Big Muscle, StrongDC++
template<class T, int ctrlId>
class TypedTreeListViewCtrl : public TypedListViewCtrl<T, ctrlId> 
{
public:

	TypedTreeListViewCtrl() { }
	~TypedTreeListViewCtrl() { states.Destroy(); }

	typedef TypedTreeListViewCtrl<T, ctrlId> thisClass;
	typedef TypedListViewCtrl<T, ctrlId> baseClass;
	
	// TODO: rewrite to hash_map
	typedef FlyLinkVector<T*> TreeItem;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButton)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP();


	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		states.CreateFromImage(IDB_STATE, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		SetImageList(states, LVSIL_STATE); 

		bHandled = FALSE;
		return 0;
	}

	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		CPoint pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		LVHITTESTINFO lvhti;
		lvhti.pt = pt;

		int pos = SubItemHitTest(&lvhti);
		if (pos != -1) {
			CRect rect;
			GetItemRect(pos, rect, LVIR_ICON);
			
			if (pt.x < rect.left) {
				T* i = (T*)getItemData(pos);
				if(i && i->collapsed) {
					Expand(i, pos);
				} else {
					Collapse(i, pos);
				}
			}
		}

		bHandled = false;
		return 0;
	} 

	void Collapse(T* s, int a) {
		for(T::Iter i = s->subItems.begin(); i != s->subItems.end(); ++i) {
			deleteItem(*i);
		}
		s->collapsed = true;
		SetItemState(a, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
	}

	void Expand(T* s, int a) {
		if(s->subItems.size() > (size_t)(uniqueMainItem ? 1 : 0)) {
			s->collapsed = false;
			for(T::Iter i = s->subItems.begin(); i != s->subItems.end(); ++i) {
				insertSubItem(*i, a + 1);
			}
			SetItemState(a, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			resort();
		}
	}

	void insertSubItem(T* i, int idx) {
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_INDENT;
		lvi.iItem = idx;
		lvi.iSubItem = 0;
		lvi.iIndent = 1;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage = i->imageIndex();
		lvi.lParam = (LPARAM)i;
		lvi.state = 0;
		lvi.stateMask = 0;
		InsertItem(&lvi);
	}

	struct TStringComp {
		TStringComp(const tstring& s) : a(s) { }
		bool operator()(T* b) const { return b->getGroupingString() == a; }
		const tstring& a;
	private:
		TStringComp& operator=(const TStringComp&);
	};

	T* findMainItem(const tstring& groupingString) const {
		TreeItem::const_iterator j = find_if(mainItems.begin(), mainItems.end(), TStringComp(groupingString));
		return j != mainItems.end() ? *j : NULL;
	}

	void insertGroupedItem(T* item, bool autoExpand, bool extra = false) {
		T* mainItem = NULL;
		
		if(!extra)
			mainItem = findMainItem(item->getGroupingString());

		int pos = -1;

		if(mainItem == NULL) {
			mainItem = item->createMainItem();
			mainItems.push_back(mainItem);

			mainItem->main = NULL; // ensure that mainItem of this item is really NULL
			pos = insertItem(getSortPos(mainItem), mainItem, mainItem->imageIndex());

			if(mainItem != item) {
				uniqueMainItem = true;
			} else {
				uniqueMainItem = false;
				return;
			}
		} else 
			pos = findItem(mainItem);

		mainItem->subItems.push_back(item);
		item->main = mainItem;
		item->updateMainItem();

		if(pos != -1) {
			size_t totalSubItems = mainItem->subItems.size();
			if(totalSubItems == (size_t)(uniqueMainItem ? 2 : 1)) {
				if(autoExpand){
					SetItemState(pos, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
					mainItem->collapsed = false;
					insertSubItem(item, pos + totalSubItems);
				} else {
					SetItemState(pos, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
				}
			} else if(!mainItem->collapsed) {
				insertSubItem(item, pos + totalSubItems);
			}
			updateItem(pos);
		}
	}

	void removeMainItem(T* s) {
		for(T::List::iterator i = s->subItems.begin(); i != s->subItems.end(); ++i) {
			deleteItem(*i);
			delete *i;
		}
		s->subItems.clear();
		mainItems.erase_and_check(s);
	}

	void removeGroupedItem(T* s, bool removeFromMemory = true) {
		if(!s->main) {
			removeMainItem(s);
		} else {
			T::List::iterator n = find(s->main->subItems.begin(), s->main->subItems.end(), s);
			if(n != s->main->subItems.end()) {
				s->main->subItems.erase(n);
			}
			if(uniqueMainItem) {
				if(s->main->subItems.size() == 1) {
					if(!s->main->collapsed) {
						s->main->collapsed = true;
						deleteItem(s->main->subItems.front());
					}
					SetItemState(findItem(s->main), INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
				} else if(s->main->subItems.empty()) {
					T* main = s->main;
					removeMainItem(main);
					deleteItem(main);
					s->main = NULL;
					delete main;
				}
			} else {
				if(s->main->subItems.empty()) {
					SetItemState(findItem(s->main), INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);
				}
			}
			if(s->main) {
				s->updateMainItem();
				updateItem(s->main);
			}
		}

		deleteItem(s);

		if(removeFromMemory)
			delete s;
	}

	void deleteAllItems() {
		// HACK: ugly hack but at least it doesn't crash and there's no memory leak
		for(TreeItem::iterator i = mainItems.begin(); i != mainItems.end(); ++i) {
			for(T::List::iterator j = (*i)->subItems.begin(); j != (*i)->subItems.end(); ++j) {
				deleteItem(*j);
				delete *j;
			}
			deleteItem(*i);
			delete *i;
		}
		for(int i = 0; i < GetItemCount(); i++) {
			T* si = getItemData(i);
			dcassert(si->subItems.empty());
			delete si;
		}

 		mainItems.clear();
		DeleteAllItems();
	}

	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem != getSortColumn()) {
			setAscending(true);
			setSortColumn(l->iSubItem);
		} else if(isAscending()) {
			setAscending(false);
		} else {
			setSortColumn(-1);
		}		
		resort();
		return 0;
	}
	void resort() {
		if(getSortColumn() != -1) {
			SortItems(&compareFunc, (LPARAM)this);
		}
	}
	int getSortPos(T* a) {
		int high = GetItemCount();
		if((getSortColumn() == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		T* b = NULL;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = getItemData(mid);
			comp = compareItems(a, b, getSortColumn());
			
			if(!isAscending())
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
				low = mid + 1;
			} else if(comp == 2){
				if(isAscending())
					low = mid + 1;
				else
					high = mid -1;
			} else if(comp == -2){
				if(!isAscending())
					low = mid + 1;
				else
					high = mid -1;
			}
		}

		comp = compareItems(a, b, getSortColumn());
		if(!isAscending())
			comp = -comp;
		if(comp > 0)
			mid++;

		return mid;
	}

   	TreeItem mainItems;

private:
	CImageList states;
	bool uniqueMainItem;
		
	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		int result = compareItems((T*)lParam1, (T*)lParam2, t->getRealSortColumn());

		if(result == 2)
			result = (t->isAscending() ? 1 : -1);
		else if(result == -2)
			result = (t->isAscending() ? -1 : 1);

		return (t->isAscending() ? result : -result);
	}

	static int compareItems(const T* a, const T* b, int col) {
		// Copyright (C) Liny, RevConnect

		// both are children
		if(a->main && b->main){
			// different parent
			if(a->main != b->main)
				return compareItems(a->main, b->main, col);			
		}else{
			if(a->main == b)
				return 2;  // a should be displayed below b

			if(b->main == a)
				return -2; // b should be displayed below a

			if(a->main)
				return compareItems(a->main, b, col);	

			if(b->main)
				return compareItems(a, b->main, col);	
		}

		return T::compareItems(a, b, col);
	}
};

#endif // !defined(TYPED_LIST_VIEW_CTRL_H)

/**
 * @file
 * $Id: TypedListViewCtrl.h,v 1.6 2008/03/10 07:51:06 alexey Exp $
 */
