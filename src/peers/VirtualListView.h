#pragma once
#include "../windows/ListViewArrows.h"
#include "Iterator.h"

namespace LV {

	struct Column {

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

	struct ColumnSettings {
		SettingsManager::StrSetting columnOrder;
		SettingsManager::StrSetting columnWidths;
		SettingsManager::StrSetting columnVisibility;
	};

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

	typedef vector<ColumnInfo*> ColumnList;
	typedef ColumnList::const_iterator ColumnIter;

	template <class T> class ListViewListener {
	public:
		virtual const T* getItem(int index) = 0;
		virtual tstring getText(const T* item, int columnIndex) const = 0;
		virtual int getItemImage(const T* item) const = 0;
	};

	template <class T, class listT> class VirtualListView : 
	public CWindowImpl<VirtualListView<T, listT>, 
		CListViewCtrl, 
		CControlWinTraits>,
		public ListViewArrows<VirtualListView<T, listT> >,
		public Iterable<listT>
	{
	public:
		typedef VirtualListView<T, listT> thisClass;
		typedef CListViewCtrl baseClass;
		typedef ListViewArrows<thisClass> arrowBase;

		VirtualListView(ListViewListener<T>* listener) : 
			m_listener(listener),
			sortColumn(-1), 
			sortAscending(true), 
			hBrBg(WinUtil::bgBrush), 
			leftMargin(0)
		  { 
		  }
		~VirtualListView() { for_each(columnList.begin(), columnList.end(), DeleteFunction()); }

		BEGIN_MSG_MAP(thisClass)
			MESSAGE_HANDLER(WM_MENUCOMMAND, onHeaderMenu)
			MESSAGE_HANDLER(WM_CHAR, onChar)
			MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
			MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
			CHAIN_MSG_MAP(arrowBase)
		END_MSG_MAP();

		bool getSelection(vector<listT>& selection) const {
			selection.clear();
			int i = -1;
			while ((i = GetNextItem(i, LVNI_SELECTED)) != -1) {
				const T *item = getItem(i);
				if (item) {
					selection.push_back(item);
				}
			}
			return !selection.empty();
		}

		template <size_t size> void InitColumns(const Column (&columns)[size]) {
			Column tempColumns[size];
			memcpy(tempColumns, columns, sizeof(Column) * size);
			int temp[size];
			// load widths
			for (int i = 0; i < size; ++i) temp[i] = columns[i].size;
			//WinUtil::splitTokens(temp, SettingsManager::getInstance()->get(listSettings.columnWidths), size);
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
			//WinUtil::splitTokens(temp, SettingsManager::getInstance()->get(listSettings.columnOrder), size);
			setColumnOrderArray(size, temp);
			// load visibility
			for (int i = 0; i < size; ++i) temp[i] = columns[i].visible;
			//WinUtil::splitTokens(temp, SettingsManager::getInstance()->get(listSettings.columnVisibility), size);
			for (int i = 0; i < size; ++i) {
				if (!temp[i]) {
					columnList[i]->m_visible = false;
					removeColumn(columnList[i]);
				}
			}
			updateColumnIndexes();
		}

		template <size_t size> void InsertColumns(const Column (&columns)[size]) {
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

		tstring tmp;

		LRESULT onGetDispInfo(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
			NMLVDISPINFO* di = (NMLVDISPINFO*)pnmh;
			if (di->item.mask & (LVIF_TEXT|LVIF_IMAGE)) {
				const T* item = getItem(di->item.iItem);
				if (item) {
					if (di->item.mask & LVIF_TEXT) {
						tmp = m_listener->getText(item, columnIndexes[di->item.iSubItem]);
						di->item.pszText = const_cast<TCHAR*>(tmp.c_str());
					}
					if (di->item.mask & LVIF_IMAGE) {
						di->item.iImage = m_listener->getItemImage(item);
					}
				}
			}
			return 0;
		}

		ColumnInfo* findColumnInfo(int columnId) {
			for (ColumnList::iterator i = columnList.begin(); i != columnList.end(); ++i) {
				ColumnInfo* ci = *i;
				if (ci->m_subItem == columnId) {
					return ci;
				}
			}
			return NULL;
		}

		LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
			if (BOOLSETTING(SHOW_INFOTIPS)) {
				NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*) pnmh;
				const T* item = getItem(pInfoTip->iItem);
				if (item) {
					const bool columnHeader = (GetWindowLong(GWL_STYLE) & LVS_NOCOLUMNHEADER) == 0;
					tstring infoTip;
					infoTip.reserve(columnList.size() * 48);
					for (size_t i = 0; i < columnIndexes.size(); ++i) {
						tstring buffer = m_listener->getText(item, columnIndexes[i]);
						if (!buffer.empty()) {
							if (!infoTip.empty()) {
								infoTip += _T("\r\n");
							}
							if (columnHeader) {
								ColumnInfo *ci = findColumnInfo(columnIndexes[i]);
								if (ci != NULL) {
									infoTip += ci->m_name;
									infoTip += _T(": ");
								}
							}
							infoTip += buffer;
						}
					}
					for (ColumnList::iterator i = columnList.begin(); i != columnList.end(); ++i) {
						ColumnInfo* ci = *i;
						if (find(columnIndexes.begin(), columnIndexes.end(), ci->m_subItem) == columnIndexes.end()) {
							tstring buffer = m_listener->getText(item, ci->m_subItem);
							if (!buffer.empty()) {
								if (!infoTip.empty()) {
									infoTip += _T("\r\n");
								}
								if (columnHeader) {
									infoTip += ci->m_name;
									infoTip += _T(": ");
								}
								infoTip += buffer;
							}
						}
					}
					_tcsncpy(pInfoTip->pszText, infoTip.c_str(), pInfoTip->cchTextMax);
					pInfoTip->pszText[pInfoTip->cchTextMax - 1] = '\0';
				}
			}
			return 0;
		}

#if 0
		int insertItem(T* item, int image) {
			return insertItem(getSortPos(item), item, image);
		}
		int insertItem(int i, T* item, int image) {
			return InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i, 
				LPSTR_TEXTCALLBACK, 0, 0, image, (LPARAM)item);
		}

		//T* getItemData(int iItem) const { return (T*)GetItemData(iItem); }
		//T* getSelectedItem() { return (GetSelectedCount() > 0 ? getItemData(GetNextItem(-1, LVNI_SELECTED)) : NULL); }

		int findItem(T* item) { 
			LVFINDINFO fi = { LVFI_PARAM, NULL, (LPARAM)item };
			return FindItem(&fi, -1);
		}
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
				(const_cast<T*>(getItem(i))->*func)(param);
		}

		void forEach(void (T::*func)()) {
			int n = GetItemCount();
			for(int i = 0; i < n; ++i)
				(getItemData(i)->*func)();
		}
		void forEachSelected(void (T::*func)()) {
			int i = -1;
			while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
				(const_cast<T*>(getItem(i))->*func)();
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
			RedrawItems(i, i);
		}
		//void updateItem(T* item) { int i = findItem(item); if(i != -1) updateItem(i); }
		//void deleteItem(T* item) { int i = findItem(item); if(i != -1) DeleteItem(i); }

		void setSortColumn(int aSortColumn) {
			sortColumn = aSortColumn;
			updateArrow();
		}
		int getSortColumn() const { return sortColumn; }
		int getRealSortColumn() const { return columnIndexes[sortColumn]; }
		bool isAscending() const { return sortAscending; }
		void setAscending(bool s) {
			sortAscending = s;
			updateArrow();
		}

		int InsertColumn(int nCol, const tstring &columnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1 ){
			if(nWidth == 0) 
				nWidth = 80;
			columnList.push_back(new ColumnInfo(columnHeading, nCol, nFormat, nWidth, nSubItem));
			columnIndexes.push_back(uint8_t(nCol));
			return CListViewCtrl::InsertColumn(nCol, columnHeading.c_str(), nFormat, nWidth, nSubItem);
		}

		void showMenu(POINT &pt){
			CMenu headerMenu;
			headerMenu.CreatePopupMenu();
			MENUINFO inf;
			inf.cbSize = sizeof(MENUINFO);
			inf.fMask = MIM_STYLE;
			inf.dwStyle = MNS_NOTIFYBYPOS;
			headerMenu.SetMenuInfo(&inf);
			for (ColumnIter i = columnList.begin(); i != columnList.end(); ++i) {
				ColumnInfo* c = *i;
				UINT flags = MF_STRING;
				if (c->m_visible) {
					flags |= MF_CHECKED;
				}
				headerMenu.AppendMenu(flags, IDC_HEADER_MENU, c->m_name.c_str());
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
			ci->m_visible = !ci->m_visible;

			SetRedraw(FALSE);

			if (!ci->m_visible){
				removeColumn(ci);
			}
			else {
				if (ci->m_width == 0) 
					ci->m_width = 80;
				CListViewCtrl::InsertColumn(ci->m_pos, ci->m_name.c_str(),
					0, //ci->format, 
					ci->m_width, static_cast<int>(wParam));
				LVCOLUMN lvcl = { 0 };
				lvcl.mask = LVCF_ORDER;
				lvcl.iOrder = ci->m_pos;
				SetColumn(ci->m_pos, &lvcl);
#if 0
				for(int i = 0; i < GetItemCount(); ++i) {
					LVITEM lvItem;
					lvItem.iItem = i;
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_IMAGE | LVIF_PARAM;
					GetItem(&lvItem);
					//AV: lvItem.iImage = ((T*)lvItem.lParam)->imageIndex();
					SetItem(&lvItem);
					updateItem(i);
				}
#endif
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

		void saveHeaderOrder(const ColumnSettings& listSettings) {
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
		//inline uint8_t findColumn(int col) const { return columnIndexes[col]; }	

		virtual size_t copyTo(listT* dest, size_t size) const {
			if (dest == NULL) {
				return GetSelectedCount();
			}
			else {
				size_t index = 0;
				int i = -1;
				while (index < size && (i = GetNextItem(i, LVNI_SELECTED)) != -1) {
					const T* item = getItem(i);
					if (item) {
						dest[index++] = item;
					}
				}
				return index;
			}
		}

	private:
		VirtualListView(const VirtualListView&); /* no copy constructor */
		int sortColumn;
		bool sortAscending;
		int leftMargin;
		HBRUSH hBrBg;
		ListViewListener<T>* const m_listener;

		const T* getItem(int index) const {
			return m_listener->getItem(index);
		}

#if 0
		static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
			thisClass* t = (thisClass*)lParamSort;
			int result = T::compareItems((T*)lParam1, (T*)lParam2, t->getRealSortColumn());
			return (t->sortAscending ? result : -result);
		}
#endif

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
#if 0
					for(int i = 0; i < GetItemCount(); ++i) {
						LVITEM lvItem;
						lvItem.iItem = i;
						lvItem.iSubItem = 0;
						lvItem.mask = LVIF_PARAM | LVIF_IMAGE;
						GetItem(&lvItem);
						//AV: lvItem.iImage = ((T*)lvItem.lParam)->imageIndex();
						SetItem(&lvItem);
					}
#endif
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

}
