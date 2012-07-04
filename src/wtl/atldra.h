// This is a part of the Device Resolution Aware Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Device Resolution Aware Library and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Device Resolution Aware Library product.

#ifndef __DEVICERESOLUTIONAWARE_H__
#define __DEVICERESOLUTIONAWARE_H__

#pragma once

#ifndef _WIN32_WCE
	#error atldra.h is only supported on Windows CE platforms.
#endif // _WIN32_WCE

#ifndef __cplusplus
	#error DRA requires C++ compilation (use a .cpp suffix)
#endif

#include <windows.h>
#include <commctrl.h>
#include <altcecrt.h>

namespace DRA
{

////////////////////////////////////////////////////////////////////////////////
// HIDPI functions and constants.
////////////////////////////////////////////////////////////////////////////////

#ifndef _DRA_ALTERNATE_DPI
const int HIDPI = 96;
#else
const int HIDPI = _DRA_ALTERNATE_DPI;
#endif

const unsigned int ILC_COLORMASK = 0x000000FE;

//
// The two inlines HIDPISIGN and HIDPIABS are there to ensure correct rounding
// for negative numbers passed into HIDPIMulDiv as x (we want -1.5 to round 
// to -1, 2.5 to round to 2, etc).  So we use the absolute value of x, and then 
// multiply the result by the sign of x.  Y and z should never be negative, as 
// y is the dpi of the device (presumably 192 or 96), and z is always 96, as 
// that is our original dpi we developed on.
//

inline int HIDPISIGN(int x)
{
	return (((x)<0)?-1:1);
}

inline int HIDPIABS(int x)
{
	return (((x)<0)?-(x):x);
}

inline int HIDPIMulDiv(int x, int y, int z)
{
	return ((((HIDPIABS(x)*(y))+((z)>>1))/(z))*HIDPISIGN(x));
}

//
// Cached values of GetDeviceCaps(LOGPIXELSX/Y) for the screen DC.
//

inline int GetScreenCaps(int nIndex)
{
	// Get the DC for the entire screen
	HDC hDC = ::GetDC(NULL);
	if(hDC == NULL)
	{
		_ASSERTE(hDC != NULL);
		return -1;
	}

	int i = ::GetDeviceCaps(hDC, nIndex);
	::ReleaseDC(NULL, hDC);
	return i;
}

static inline int LogPixelsX()
{
#ifndef _DRA_ADJUSTABLE_RESOLUTION
	static int x = GetScreenCaps(LOGPIXELSX);
#else
	int x = GetScreenCaps(LOGPIXELSX);
#endif
	return x;
}

static inline int LogPixelsY()
{
#ifndef _DRA_ADJUSTABLE_RESOLUTION
	static int y = GetScreenCaps(LOGPIXELSY);
#else
	int y = GetScreenCaps(LOGPIXELSY);
#endif
	return y;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: GetDisplayMode
//
// PURPOSE: used to determined if the display is currently configured as 
//     portrait, square, or landscape.
//
// ON ENTRY:
//     No parameters
//
// ON EXIT:
//     Returns the appropraite value from the DisplayMode enum based on the
//     current display resolution.
//

enum DisplayMode
{
	Landscape = -1,
	Square = 0,
	Portrait = 1
};

inline DisplayMode GetDisplayMode()

{
	int nWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int nHeight = ::GetSystemMetrics(SM_CYSCREEN);

	if(nHeight > nWidth)
	{
		return Portrait;
	}

	if(nHeight < nWidth)
	{
		return Landscape;
	}

   	return Square;
}

//
// Scaling inlines.
//

inline int SCALEX(int argX, int nLogPixelsX = LogPixelsX())
{
	return HIDPIMulDiv(argX, nLogPixelsX, HIDPI);
}

inline int SCALEY(int argY, int nLogPixelsY = LogPixelsY())
{
	return HIDPIMulDiv(argY, nLogPixelsY, HIDPI);
}

inline int UNSCALEX(int argX, int nLogPixelsX = LogPixelsX())
{
	return HIDPIMulDiv(argX, HIDPI, nLogPixelsX);
}

inline int UNSCALEY(int argY, int nLogPixelsY = LogPixelsY())
{
	return HIDPIMulDiv(argY, HIDPI, nLogPixelsY);
}

inline void SCALERECT(RECT& rc)
{
#ifndef _DRA_ADJUSTABLE_RESOLUTION
	rc.left = SCALEX(rc.left);
	rc.right = SCALEX(rc.right);
	rc.top = SCALEY(rc.top);
	rc.bottom = SCALEY(rc.bottom);
#else // _DRA_ADJUSTABLE_RESOLUTION
	int nLogPixelsX = LogPixelsX();
	int nLogPixelsY = LogPixelsY();
	rc.left = SCALEX(rc.left, nLogPixelsX);
	rc.right = SCALEX(rc.right, nLogPixelsX);
	rc.top = SCALEY(rc.top, nLogPixelsY);
	rc.bottom = SCALEY(rc.bottom, nLogPixelsY);
#endif // _DRA_ADJUSTABLE_RESOLUTION
}

inline void SCALEPOINT(POINT& pt)
{
	pt.x = SCALEX(pt.x);
	pt.y = SCALEY(pt.y);
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: StretchIcon
//
// PURPOSE: stretches an icon to the specified size on 4.21 devices and later.
//     On 4.20 and previous revisions of the OS, this is a no-op.
//
// ON ENTRY:
//     HICON hiconIn: the icon to stretch.
//     HICON* phiconOut: the stretched icon.
//     INT cxIcon: the desired width of the icon.
//     INT cyIcon: the desired height of the icon.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//

inline BOOL StretchIcon(
	HICON hiconIn,
	HICON* phiconOut,
	int cxIcon,
	int cyIcon)
{
	*phiconOut = NULL;
	
	HDC hdc = ::CreateCompatibleDC(NULL);
	if(hdc == NULL)
	{
		return FALSE;
	}

	BOOL bRet = FALSE;
	
	HBITMAP hbmMask = ::CreateCompatibleBitmap(hdc, cxIcon, cyIcon);
	if(hbmMask != NULL)
	{
		HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdc, hbmMask));
		if(hbmOld != NULL)
		{
			BOOL bRet2 = ::DrawIconEx(hdc, 0, 0, hiconIn, cxIcon, cyIcon, 0, NULL, DI_MASK);
			if(bRet2 != FALSE)
			{
				HBITMAP hbmImage = ::CreateBitmap(cxIcon, cyIcon, 1, ::GetDeviceCaps(hdc, BITSPIXEL), NULL);
				if(hbmImage != NULL)
				{
					hbmOld = static_cast<HBITMAP>(::SelectObject(hdc, hbmImage));
					if(hbmOld != NULL)
					{
						bRet2 = ::DrawIconEx(hdc, 0, 0, hiconIn, cxIcon, cyIcon, 0, NULL, DI_IMAGE);
						if(bRet2 != FALSE)
						{
							ICONINFO iconinfo;
							::memset(&iconinfo, 0, sizeof(iconinfo));
							iconinfo.fIcon = TRUE;
							iconinfo.hbmColor = hbmImage;
							iconinfo.hbmMask = hbmMask;
							
							*phiconOut = ::CreateIconIndirect(&iconinfo);
							if(phiconOut != NULL)
							{
								bRet = TRUE;
							}
						}
					}
// Free the resources, but don't need to put restore original objects for deleted DCs.
					::DeleteObject(hbmImage);
				}
			}
		}
		::DeleteObject(hbmMask);
	}
	::DeleteDC(hdc);

	return bRet;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: GetBitmapLogPixels
//
// PURPOSE: retrieves the DPI fields of the specified bitmap.
//
// ON ENTRY:
//     HINSTANCE hinst: the HINSTANCE of the bitmap resource.
//     LPCTSTR lpbmp: the ID of the bitmap resource.  The MAKEINTRESOURCE 
//         macro can be used for integer IDs.
//     INT* pnLogPixelsX: the returned value for the horizontal DPI field of
//         the bitmap.  This value is never less than 96.
//     INT* pnLogPixelsY: the returned value for the vertical DPI field of
//         the bitmap.  This value is never less than 96.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//     

inline BOOL GetBitmapLogPixels(
	HINSTANCE hinst,
	LPCTSTR lpbmp,
	int* pnLogPixelsX,
	int* pnLogPixelsY)
{
	*pnLogPixelsX = 0;
	*pnLogPixelsY = 0;

	HRSRC hResource = ::FindResource(hinst, lpbmp, RT_BITMAP);
	if(!hResource)
	{
		return FALSE;
	}
	
	HGLOBAL hResourceBitmap = ::LoadResource(hinst, hResource);
	if(!hResourceBitmap)
	{
		return FALSE;
	}
	
	BITMAPINFO* pBitmapInfo = static_cast<BITMAPINFO*>(::LockResource(hResourceBitmap));
	if(!pBitmapInfo)
	{
		return FALSE;
	}

	// There are at least three values of PelsPerMeter used for 96 DPI bitmap:
	//   0    - the bitmap just simply doesn't set this value
	//   2834 - 72 DPI (some editors just always put this in by default)
	//   3780 - 96 DPI
	// So any value of PelsPerMeter under 3780 should be treated as 96 DPI bitmap.

	int nPelsPerMeterX = (pBitmapInfo->bmiHeader.biXPelsPerMeter < 3780) ? 3780 : pBitmapInfo->bmiHeader.biXPelsPerMeter;
	int nPelsPerMeterY = (pBitmapInfo->bmiHeader.biYPelsPerMeter < 3780) ? 3780 : pBitmapInfo->bmiHeader.biYPelsPerMeter;

	// The formula for converting PelsPerMeter to LogPixels(DPI) is:
	//   LogPixels = PelsPerMeter / 39.37
	//   ( PelsPerMeter : Pixels per meter )
	//   ( LogPixels    : Pixels per inch  )
	// Note: We need to round up, which is why 19.68 is added (half of 39.37).
	// All values are multiplied by 100 to avoid converion to float and back to int.

	*pnLogPixelsX = static_cast<int>((nPelsPerMeterX * 100 + 1968) / 3937);
	*pnLogPixelsY = static_cast<int>((nPelsPerMeterY * 100 + 1968) / 3937);

	// There is only one system resource behind hResource, hResourceBitmap, 
	// and pBitmapInfo, which needs to be freed by calling DeleteObject
	::DeleteObject(static_cast<HGDIOBJ>(hResourceBitmap));

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: ImageList_StretchBitmap
//
// PURPOSE: Stretches a bitmap containing a grid of images.  There are 
//     cImagesX images per row and cImagesY rows per bitmap.  Each image is 
//     scaled individually, so that there are no artifacts with non-integral 
//     scaling factors.  If the bitmap contains only one image, set cImagesX
//     and cImagesY to 1.
//
// ON ENTRY:
//     HBITMAP hbmIn: the bitmap to be scaled.
//     HBITMAP* phbmOut: an HBITMAP pointer that will be set to the scaled bitmap.  Use DeleteObject to free the handle when it is no longer needed.
//     INT cxDstImg: the width of each image after scaling.
//     INT cyDstImg: the height of each image after scaling.
//     INT cImagesX: the number of images per row. This value should
//         evenly divide the width of the bitmap.
//     INT cImagesY: the number of rows in the bitmap. This value should 
//         evenly divide the height of the bitmap.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.  If the method fails phbmOut will not be set.
//

inline BOOL ImageList_StretchBitmap(
	HBITMAP hbmIn,
	HBITMAP* phbmOut,
	int cxDstImg,
	int cyDstImg,
	int cImagesX,
	int cImagesY)
{
	if(	phbmOut == NULL || 
		(cxDstImg == 0 && cyDstImg == 0) || 
		(cImagesX == 0 || cImagesY == 0))
	{
		return FALSE;
	}

	BITMAP bm;
	int nSize = ::GetObject(hbmIn, sizeof(bm), &bm);
	if(nSize != sizeof(bm))
	{
		return FALSE;
	}

	// If you hit this ASSERT, that mean your passed in image count in row and
	//   the column number of images is not correct.
	_ASSERTE(((bm.bmWidth % cImagesX) == 0) && ((bm.bmHeight % cImagesY) == 0));

	int cxSrcImg = bm.bmWidth / cImagesX;
	int cySrcImg = bm.bmHeight / cImagesY;

	if(cxSrcImg == cxDstImg && cySrcImg == cyDstImg)
	{
		return TRUE;
	}

	if(cxDstImg == 0)
	{
		cxDstImg = HIDPIMulDiv(cyDstImg, cxSrcImg, cySrcImg);
	}
	else if(cyDstImg == 0)
	{
		cyDstImg = HIDPIMulDiv(cxDstImg, cySrcImg, cxSrcImg);
	}

	BOOL bRet = TRUE;
	HBITMAP hbmNew;

	HDC hdcSrc = ::CreateCompatibleDC(NULL);
	if(hdcSrc != NULL)
	{
		HDC hdcDst = ::CreateCompatibleDC(NULL);
		if(hdcDst != NULL)
		{
			HDC hdcScreen = ::GetDC(NULL);
			if(hdcScreen != NULL)
			{
		
				HBITMAP hbmOldSrc = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbmIn));
				if(hbmOldSrc != NULL)
				{
					hbmNew = ::CreateCompatibleBitmap(hdcScreen, cxDstImg * cImagesX, cyDstImg * cImagesY);
					if(hbmNew != NULL)
					{
						HBITMAP hbmOldDst = static_cast<HBITMAP>(::SelectObject(hdcDst, hbmNew));
						if(hbmOldDst != NULL)
						{
							for(int j = 0, yDest = 0, yBmp = 0;
								(j < cImagesY) && (bRet != FALSE);
								++j, yDest += cyDstImg, yBmp += cySrcImg)
							{
								for(int i = 0, xDest = 0, xBmp = 0;
									(i < cImagesX) && (bRet != FALSE);
									++i, xDest += cxDstImg, xBmp += cxSrcImg)
								{
									bRet = ::StretchBlt(
										hdcDst, 
										xDest, 
										yDest, 
										cxDstImg, 
										cyDstImg,
										hdcSrc, 
										xBmp, 
										yBmp, 
										cxSrcImg, 
										cySrcImg,
										SRCCOPY);
								}
							}
						}
					}
				}
// Free the resources, but don't need to put restore original objects for deleted DCs.
				::ReleaseDC(NULL, hdcScreen);
			}
			::DeleteDC(hdcDst);
		}
		::DeleteDC(hdcSrc);
	}

	if(bRet != FALSE)
	{
		*phbmOut = hbmNew;
	}
	
	return bRet;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: ImageList_LoadImage
//
// PURPOSE: This function operates identically to ImageList_LoadImage, except
//     that it first checks the DPI fields of the bitmap (using 
//     GetBitmapLogPixels); compares it to the DPI of the screen
//     (using LogPixelsX() and LogPixelsY()), and performs scaling
//     (using ImageList_StretchBitmap) if the values are different.
//
// ON ENTRY:
//     See the MSDN documentation for ImageList_LoadImage.
//
// ON EXIT:
//     See the MSDN documentation for ImageList_LoadImage.
//

inline HIMAGELIST ImageList_LoadImage(
	HINSTANCE hinst,
	LPCTSTR lpbmp,
	int cx,
	int cGrow,
	COLORREF crMask,
	UINT uType,
	UINT uFlags)
{
	if(	uType != IMAGE_BITMAP ||  // Image type is not IMAGE_BITMAP
		cx == 0)                  // Caller doesn't care about the dimensions of the image - assumes the ones in the file
	{
		return ::ImageList_LoadImage(hinst, lpbmp, cx, cGrow, crMask, uType, uFlags);
	}

	int BmpLogPixelsX;
	int BmpLogPixelsY;
	if(!GetBitmapLogPixels(hinst, lpbmp, &BmpLogPixelsX, &BmpLogPixelsY))
	{
		return NULL;
	}

	HIMAGELIST piml = NULL;
	
	HBITMAP hbmImage = static_cast<HBITMAP>(::LoadImage(hinst, lpbmp, uType, 0, 0, uFlags));
	if(hbmImage == NULL)
	{
		goto cleanup;
	}

	BITMAP bm;
	int nSize = ::GetObject(hbmImage, sizeof(bm), &bm);
	if(nSize != sizeof(bm))
	{
		goto cleanup;
	}

	if(BmpLogPixelsX == LogPixelsX())
	{
		// do not need to scale the bitmap
		piml = ::ImageList_LoadImage(hinst, lpbmp, cx, cGrow, crMask, uType, uFlags);
		goto cleanup;
	}

	int cxImage = HIDPIMulDiv(cx, LogPixelsX(), BmpLogPixelsX);

	// Bitmap width should be multiple integral of image width.
	// If not, that means either your bitmap is wrong or passed in cx is wrong.
	_ASSERTE((bm.bmWidth % cx) == 0);

	int cImages = bm.bmWidth / cx;

	int cyImage = HIDPIMulDiv(bm.bmHeight, LogPixelsY(), BmpLogPixelsY);

	HBITMAP hbmStretched;
	BOOL bRet;
	if((LogPixelsX() % BmpLogPixelsX) == 0)
	{
		bRet = ImageList_StretchBitmap(hbmImage, &hbmStretched, cxImage * cImages, cyImage, 1, 1);
	}
	else
	{
		// Here means the DPI is not integral multiple of standard DPI (96DPI).
		// So if we stretch entire bitmap together, we are not sure each indivisual
		//   image will be stretch to right place. It is controled by StretchBlt().
		//   (for example, a 16 pixel icon, the first one might be stretch to 22 pixels
		//    and next one might be stretched to 20 pixels)
		// What we have to do here is stretching indivisual image separately to make sure
		//   every one is stretched properly.
		bRet = ImageList_StretchBitmap(hbmImage, &hbmStretched, cxImage, cyImage, cImages, 1);
	}
	if(bRet == FALSE)
	{
		goto cleanup;
	}

	UINT flags = 0;
	
	// ILC_MASK is important for supporting CLR_DEFAULT
	if(crMask != CLR_NONE)
	{
		flags |= ILC_MASK;
	}
	
	// ILC_COLORMASK bits are important if we ever want to Merge ImageLists
	if(bm.bmBits)
	{
		flags |= (bm.bmBitsPixel & ILC_COLORMASK);
	}

	// bitmap MUST be de-selected from the DC
	// create the image list of the size asked for.
	piml = ::ImageList_Create(cxImage, cyImage, flags, cImages, cGrow);

	if(piml)
	{
		int added;

		if(crMask == CLR_NONE)
		{
			added = ::ImageList_Add(piml, hbmStretched, NULL);
		}
		else
		{
			added = ::ImageList_AddMasked(piml, hbmStretched, crMask);
		}

		if(added < 0)
		{
			::ImageList_Destroy(piml);
			piml = NULL;
		}
	}
	
	::DeleteObject(hbmStretched);

cleanup:
	::DeleteObject(hbmImage);
	return piml;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: ImageList_ReplaceIcon
//
// PURPOSE: Replaces an icon in an ImageList, scaling it from its original size
//          to the size of the images in the ImageList.
//
// ON ENTRY:
//     See the MSDN documentation for ImageList_ReplaceIcon.
//
// ON EXIT:
//     See the MSDN documentation for ImageList_ReplaceIcon.
//

inline int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
	int cxIcon, cyIcon;
	BOOL bRet = ::ImageList_GetIconSize(himl, &cxIcon, &cyIcon);
	if(bRet == FALSE)
	{
		return -1; // per MSDN documentation for ImageList_ReplaceIcon.
	}
	
	HICON hiconStretched;
	StretchIcon(hicon, &hiconStretched, cxIcon, cyIcon);
	
	int iRet;
	
	if(hiconStretched != NULL)
	{
		iRet = ::ImageList_ReplaceIcon(himl, i, hiconStretched);
		::DestroyIcon(hiconStretched);
	}
	else
	{
		iRet = ::ImageList_ReplaceIcon(himl, i, hicon);
	}

	return iRet;
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: ImageList_AddIcon
//
// PURPOSE: Adds an icon to an ImageList, scaling it from its original size
//          to the size of the images in the ImageList.
//
// ON ENTRY:
//     See the MSDN documentation for ImageList_AddIcon.
//
// ON EXIT:
//     See the MSDN documentation for ImageList_AddIcon.
//

// ImageList_AddIcon is defined as a macro in commctrl.h
#ifdef ImageList_AddIcon
#undef ImageList_AddIcon
#endif

inline int ImageList_AddIcon(HIMAGELIST himl, HICON hicon)
{
	return DRA::ImageList_ReplaceIcon(himl, -1, hicon);
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: Rectangle
//
// PURPOSE: Draws a rectangle using the currently selected pen.  Drawing occurs
//    completely within the drawing rectangle (the rectangle has an "inside 
//    frame" drawing style).
//
// ON ENTRY:
//     HDC hdc: the display context of the drawing surface.
//     INT nLeft: left bound of rectangle
//     INT nTop: top bound of rectangle
//     INT nRight: right bound of rectangle plus one.
//     INT nBottom: bottom bound of rectangle plus one.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//

inline BOOL Rectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom)
{
	// Obtain current pen thickness
	HPEN hpenSel = static_cast<HPEN>(::GetCurrentObject(hdc, OBJ_PEN));
	if(hpenSel == NULL)
	{
		return FALSE;
	}
	
	LOGPEN lpenSel;
	int iRet = ::GetObject(hpenSel, sizeof(lpenSel), &lpenSel);
	if(iRet == 0)
	{
		return FALSE;
	}

	int nOff = lpenSel.lopnWidth.x/2;
	    
	nLeft += nOff;
	nTop += nOff;
	nRight -= nOff;
	nBottom -= nOff;

	return ::Rectangle(hdc, nLeft, nTop, nRight, nBottom);
}
 
//////////////////////////////////////////////////////////////////////////////
// FUNCTION: Polyline
//
// PURPOSE: Draws a polyline using the currently selected pen.  In addition,
//     this function provides control over how the line will be drawn.
//
// ON ENTRY:
//     HDC hdc: the display context of the drawing surface.
//     const POINT* lppt: array of POINTS that specify line to draw.
//     INT cPoints: number of points in array.
//     INT nStyle: the style the pen should be drawn in.  This may be an 
//        existing pen style, such as PS_SOLID, or one of the following styles:
//
//           PS_LEFTBIAS      PS_UPBIAS        PS_UPLEFT
//           PS_RIGHTBIAS     PS_DOWNBIAS      PS_DOWNRIGHT
//
//        These styles indicate how the pen should "hang" from each line
//        segment.  By default, the pen is centered along the line, but with
//        these line styles the developer can draw lines above, below, to the
//        left or to the right of the line segment.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//

const int PS_RIGHTBIAS = 0x10;
const int PS_LEFTBIAS  = 0x20;
const int PS_DOWNBIAS  = 0x40;
const int PS_UPBIAS    = 0x80;

const int PS_DOWNRIGHT = (PS_DOWNBIAS | PS_RIGHTBIAS);
const int PS_UPLEFT    = (PS_UPBIAS | PS_LEFTBIAS);
const int PS_BIAS_MASK = (PS_RIGHTBIAS | PS_LEFTBIAS | PS_DOWNBIAS | PS_UPBIAS);

inline BOOL Polyline(HDC hdc, const POINT *lppt, int cPoints, int nStyle)
{
	// Make sure caller didn't try to get both a left and a right bias or both a down and an up bias
	_ASSERTE(!(nStyle & PS_LEFTBIAS) || !(nStyle & PS_RIGHTBIAS));
	_ASSERTE(!(nStyle & PS_UPBIAS) || !(nStyle & PS_DOWNBIAS));

	if(!(nStyle & PS_BIAS_MASK))
	{
		// No drawing bias. Draw normally
		return ::Polyline(hdc, lppt, cPoints);
	}

	// Obtain current pen thickness
	HPEN hpenSel = static_cast<HPEN>(::GetCurrentObject(hdc, OBJ_PEN));
	if(hpenSel == NULL)
	{
		return FALSE;
	}
	
	LOGPEN lpenSel;
	int iRet = ::GetObject(hpenSel, sizeof(lpenSel), &lpenSel);
	if(iRet == 0)
	{
		return FALSE;
	}

	int nThickness = lpenSel.lopnWidth.x;

	int nHOff = 0;

	if(nStyle & PS_LEFTBIAS)
	{
		nHOff = -((nThickness-1)/2);
	}

	if(nStyle & PS_RIGHTBIAS)
	{
		nHOff = nThickness/2;
	}

	int nVOff = 0;

	if(nStyle & PS_UPBIAS)
	{
		nVOff = -((nThickness-1)/2);
	}

	if(nStyle & PS_DOWNBIAS)
	{
		nVOff = nThickness/2;
	}

	BOOL bRet = TRUE;
	POINT pts[2];

	for(int i = 1; i < cPoints; ++i)
	{
		// Use the two points that specify current line segment
		::memcpy(pts, &lppt[i-1], sizeof(pts));

		if(::abs(lppt[i].x - lppt[i-1].x) <= ::abs(lppt[i].y - lppt[i-1].y))
		{
			// Shift current line segment horizontally if abs(slope) >= 1
			pts[0].x += nHOff;
			pts[1].x += nHOff;
		}
		else
		{
			// Shift current line segment vertically if abs(slope) < 1
			pts[0].y += nVOff;
			pts[1].y += nVOff;
		}

		bRet = ::Polyline(hdc, pts, sizeof(pts)/sizeof(pts[0]));
		if(bRet == FALSE)
		{
			break;
		}
	}

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////
// Orientation functions.
///////////////////////////////////////////////////////////////////////////////

//
// Called by RelayoutDialog to advance to the next item in the dialog template.
//
inline LPBYTE WalkDialogData(LPBYTE lpData)
{
	LPWORD lpWord = reinterpret_cast<LPWORD>(lpData);
	if(*lpWord == 0xFFFF)
	{
		return reinterpret_cast<LPBYTE>(lpWord + 2);
	}
	
	while(*lpWord != 0x0000)
	{
		++lpWord;
	}
	
	return reinterpret_cast<LPBYTE>(lpWord + 1);
}

//
// Post-processing step for each dialog item.
//    Static controls and buttons: change text and bitmaps.
//    Listboxes and combo boxes: ensures that the selected item is visible.
//
inline void FixupDialogItem(
	HINSTANCE hInst, 
	HWND hDlg, 
	LPDLGITEMTEMPLATE lpDlgItem, 
	LPWORD lpClass, 
	LPWORD lpData)
{
	if(lpClass[0] == 0xFFFF)
	{
		switch (lpClass[1])
		{
		case 0x0080: // button
		case 0x0082: // static
			{

			if(lpData[0] == 0xFFFF)
			{
				// NOTE - SendDlgItemMessageW return code indicating failure
			
				if((lpDlgItem->style & SS_ICON) == SS_ICON)
				{
					HICON hOld = reinterpret_cast<HICON>(::SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_GETIMAGE, IMAGE_ICON, 0));
					HICON hNew = ::LoadIcon(hInst, MAKEINTRESOURCE(lpData[1]));
					if(hNew != NULL)
					{
						::SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hNew);
						::DestroyIcon(hOld); // Only delete hOld if we get a valid hNew, as it isn't allocated here, just replaced
					}
				}
				else if((lpDlgItem->style & SS_BITMAP) == SS_BITMAP)
				{
					HBITMAP hOld = reinterpret_cast<HBITMAP>(::SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_GETIMAGE, IMAGE_BITMAP, 0));
					HBITMAP hNew = ::LoadBitmap(hInst, MAKEINTRESOURCE(lpData[1]));
					if(hNew != NULL)
					{
						::SendDlgItemMessageW(hDlg, lpDlgItem->id, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hNew);
						::DeleteObject(hOld);  // Only delete hOld if we get a valid hNew, as it isn't allocated here, just replaced
					}
				}
			}
			else // lpData[0] is not 0xFFFF (it's text).
			{
				::SetDlgItemTextW(hDlg, lpDlgItem->id, (LPCTSTR)lpData);
			}
			
			break;
			
			}

		// This forces the list box to scroll into view in case it disappeared from view during the rotation.
		case 0x0083: // list box
			{

			INT nSel = ::SendDlgItemMessageW(hDlg, lpDlgItem->id, LB_GETCURSEL, 0, 0);
			if(nSel != LB_ERR) 
			{
				::SendDlgItemMessageW(hDlg, lpDlgItem->id, LB_SETCURSEL, nSel, 0);
			}
			
			break;
			
			}

		// This forces the combo box to scroll into view in case it disappeared from view during the rotation.
		case 0x0085: // combo box
			{
				
			INT nSel = ::SendDlgItemMessageW(hDlg, lpDlgItem->id, CB_GETCURSEL, 0, 0);
			if(nSel != CB_ERR) 
			{
				::SendDlgItemMessageW(hDlg, lpDlgItem->id, CB_SETCURSEL, nSel, 0);
			}
			
			break;
			
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: RelayoutDialog
//
// PURPOSE: Re-lays out a dialog based on a dialog template.  This function
//      iterates through all the child window controls and does a SetWindowPos
//      for each.  It also does a SetWindowText for each static text control 
//      and updates the selected bitmap or icon in a static image control. 
//      This assumes that the current dialog and the new template have all the 
//      same controls, with the same IDCs.
//
// ON ENTRY:
//      HINSTANCE hInst: the hInstance of the current module.
//      HWND hDlg: the dialog to layout.
//      LPCWSTR iddTemplate: the new template for the dialog (can use 
//          the MAKEINTRESOURCE macro).
//
// ON EXIT: TRUE if success; FALSE if failure (either the iddTemplate is 
//      invalid, or there are two or more IDC_STATICs in the template).
//

inline BOOL RelayoutDialog(HINSTANCE hInst, HWND hDlg, LPCWSTR iddTemplate)
{
	HRSRC hRsrc = ::FindResource(static_cast<HMODULE>(hInst), iddTemplate, RT_DIALOG);
	if(hRsrc == NULL) 
	{
		return FALSE;
	}

	HGLOBAL hGlobal = ::LoadResource(static_cast<HMODULE>(hInst), hRsrc);
	if(hGlobal == NULL)
	{
		return FALSE;
	}

	LPBYTE lpData = reinterpret_cast<LPBYTE>(::LockResource(hGlobal));
	if(lpData == NULL)
	{
		return FALSE;
	}
	
	LPDLGTEMPLATE lpTemplate = reinterpret_cast<LPDLGTEMPLATE>(lpData);
	HDWP hDWP = ::BeginDeferWindowPos(lpTemplate->cdit);
	if(hDWP == NULL)
	{
		return FALSE;
	}

	//
	// For more information about the data structures that we are walking,
	// consult the DLGTEMPLATE and DLGITEMTEMPLATE documentation on MSDN.
	//
	lpData += sizeof(DLGTEMPLATE);
	lpData = WalkDialogData(lpData);     // menu
	lpData = WalkDialogData(lpData);     // class
	lpData = WalkDialogData(lpData);     // title

	if(lpTemplate->style & DS_SETFONT)
	{
		lpData += sizeof(WORD);          // font size.
		lpData = WalkDialogData(lpData); // font face.
	}

	INT nStatics = 0;
	BOOL bRet = TRUE;
	for(int i = 0; i < lpTemplate->cdit; i++)
	{
		lpData = reinterpret_cast<LPBYTE>(((INT)lpData + 3) & ~3);  // force to DWORD boundary.
		
		LPDLGITEMTEMPLATE lpDlgItem = reinterpret_cast<LPDLGITEMTEMPLATE>(lpData);
		
		HWND hwndCtl = ::GetDlgItem(hDlg, lpDlgItem->id);
		if(hwndCtl == NULL)
		{
			bRet = FALSE;
			continue;
		}

		//
		// Move the item around.
		//
		{
			
		RECT r;
		r.left   = lpDlgItem->x;
		r.top    = lpDlgItem->y;
		r.right  = lpDlgItem->x + lpDlgItem->cx;
		r.bottom = lpDlgItem->y + lpDlgItem->cy;
		
		BOOL bRet2 = ::MapDialogRect(hDlg, &r);
		if(bRet2 == FALSE)
		{
			bRet = FALSE;
			continue;
		}
		
		HDWP hdwp = ::DeferWindowPos(
			hDWP, 
			hwndCtl, 
			NULL, 
			r.left, 
			r.top, 
			r.right - r.left, 
			r.bottom - r.top, 
			SWP_NOZORDER);
		if(hdwp == NULL)
		{
			bRet = FALSE;
			// Note the failure but continue on anyway
		}
		
		}

		lpData += sizeof(DLGITEMTEMPLATE);
		LPWORD lpClass = reinterpret_cast<LPWORD>(lpData);
		lpData = WalkDialogData(lpData);  // class

		//
		// Do some special handling for each dialog item (changing text,
		// bitmaps, ensuring visible, etc.
		//
		FixupDialogItem(hInst, hDlg, lpDlgItem, lpClass, reinterpret_cast<LPWORD>(lpData));

		lpData = WalkDialogData(lpData);  // title        
		WORD cbExtra = *((LPWORD)lpData); // extra class data.
		lpData += (cbExtra ? cbExtra : sizeof(WORD));

		if(lpDlgItem->id == 0xFFFF)
		{
			++nStatics;
		}
	}

	// Be sure not to overwrite a FALSE value in bRet with a TRUE value from this call
	BOOL bRet2 = ::EndDeferWindowPos(hDWP);
	if(bRet2 == FALSE)
	{
		bRet = FALSE;
		
	}

	// Note failure if there was more then one item with an id of -1, as the items
	// will not have been matched up correctly.
	return nStatics < 2 ? bRet : FALSE;
}

} // namespace DRA

#endif // __DEVICERESOLUTIONAWARE_H__

/////////////////////////////////////////////////////////////////////////////
