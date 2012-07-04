/* 
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

#if !defined(AFX_IMAGEDATAOBJECT_H__7E162227_62B8_49E3_A35B_FEC3F241A78F__INCLUDED_)
#define AFX_IMAGEDATAOBJECT_H__7E162227_62B8_49E3_A35B_FEC3F241A78F__INCLUDED_

class CImageDataObject : IDataObject {
public:
	static void InsertBitmap(IRichEditOle* pRichEditOle, HBITMAP hBitmap);

private:
	ULONG	m_ulRefCnt;
	BOOL	m_bRelease;

	// The data being bassed to the richedit
	STGMEDIUM m_stgmed;
	FORMATETC m_fromat;

public:
	CImageDataObject() : m_ulRefCnt(0), m_bRelease(FALSE) {	}
	~CImageDataObject() {
		if (m_bRelease)
			::ReleaseStgMedium(&m_stgmed);
	}

	// Methods of the IUnknown interface

	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) {
		if (iid == IID_IUnknown || iid == IID_IDataObject) {
			*ppvObject = this;
			AddRef();
			return S_OK;
		} else
			return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)(void) {
		m_ulRefCnt++;
		return m_ulRefCnt;
	}
	STDMETHOD_(ULONG, Release)(void) {
		if (--m_ulRefCnt == 0) {
			delete this;
			return 0;
		}

		return m_ulRefCnt;
	}

	// Methods of the IDataObject Interface

	STDMETHOD(GetData)(FORMATETC* /*pformatetcIn*/, STGMEDIUM *pmedium) {

		pmedium->tymed = TYMED_GDI;
		pmedium->hBitmap = m_stgmed.hBitmap;
		pmedium->pUnkForRelease = NULL;

		return S_OK;
	}
	STDMETHOD(GetDataHere)(FORMATETC* /*pformatetc*/, STGMEDIUM*  /*pmedium*/) {
		return E_NOTIMPL;
	}
	STDMETHOD(QueryGetData)(FORMATETC* /*pformatetc*/) {
		return E_NOTIMPL;
	}
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* /*pformatectIn*/,FORMATETC* /*pformatetcOut*/) {
		return E_NOTIMPL;
	}
	STDMETHOD(SetData)(FORMATETC* pformatetc , STGMEDIUM*  pmedium , BOOL  fRelease ) {
		m_fromat = *pformatetc;
		m_stgmed = *pmedium;
		m_bRelease = fRelease;
		return S_OK;
	}
	STDMETHOD(EnumFormatEtc)(DWORD /*dwDirection*/, IEnumFORMATETC** /*ppenumFormatEtc*/) {
		return E_NOTIMPL;
	}
	STDMETHOD(DAdvise)(FORMATETC* /*pformatetc*/, DWORD /*advf*/, IAdviseSink* /*pAdvSink*/,
		DWORD* /*pdwConnection*/) {
		return E_NOTIMPL;
	}
	STDMETHOD(DUnadvise)(DWORD /*dwConnection*/) {
		return E_NOTIMPL;
	}
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA ** /*ppenumAdvise*/) {
		return E_NOTIMPL;
	}

	// Some Other helper functions

	void SetBitmap(HBITMAP hBitmap);
	IOleObject *GetOleObject(IOleClientSite *pOleClientSite, IStorage *pStorage);

};

#endif // !defined(AFX_IMAGEDATAOBJECT_H__7E162227_62B8_49E3_A35B_FEC3F241A78F__INCLUDED_)
