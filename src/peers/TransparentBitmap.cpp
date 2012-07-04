#include "stdafx.h"
#include "TransparentBitmap.h"

class AutoTransparentColorContainer : public TransparentColorContainer {
private:
  int m_height;
public:
  AutoTransparentColorContainer(TransparentBitmap* bitmap): m_height(bitmap->getHeight()) {}
  virtual COLORREF getTransparentColor(CDCHandle dc) const {
    return dc.GetPixel(0, m_height - 1);
  }
};

class ConstTransparentColorContainer : public TransparentColorContainer {
private:
  COLORREF m_color;
public:
  ConstTransparentColorContainer(COLORREF color): m_color(color) {}
  virtual COLORREF getTransparentColor(CDCHandle dc) const {
    return m_color;
  }
};

class PointTransparentColorContainer : public TransparentColorContainer {
private:
  POINT m_point;
public:
  PointTransparentColorContainer(POINT point): m_point(point) {}
  virtual COLORREF getTransparentColor(CDCHandle dc) const {
    return dc.GetPixel(m_point.x, m_point.y);
  }
};

TransparentBitmap::TransparentBitmap():
m_width(0),
m_height(0) {
}

bool TransparentBitmap::findDimensions() {
  BITMAP bmp;
  if (!m_bitmap.GetBitmap(&bmp)) {
    return false;
  }
  m_width = bmp.bmWidth;
  m_height = bmp.bmHeight;
  return true;
}

void TransparentBitmap::createMask(const TransparentColorContainer* container) {
  m_mask.CreateBitmap(m_width, m_height, 1, 1, NULL);
  CDC bitmapDC;
  bitmapDC.CreateCompatibleDC(NULL);
  bitmapDC.SelectBitmap(m_bitmap);
  bitmapDC.SetBkColor(container->getTransparentColor((HDC) bitmapDC));
  CDC maskDC;
  maskDC.CreateCompatibleDC(NULL);
  maskDC.SelectBitmap(m_mask);
  maskDC.BitBlt(0, 0, m_width, m_height, bitmapDC, 0, 0, SRCCOPY);
  bitmapDC.SetTextColor(RGB(255,255,255));
  bitmapDC.SetBkColor(RGB(0,0,0));
  bitmapDC.BitBlt(0, 0, m_width, m_height, maskDC, 0, 0, SRCAND);
}

bool TransparentBitmap::init(ATL::_U_STRINGorID resourceId) {
  if (!m_bitmap.LoadBitmap(resourceId)) {
    return false;
  }
  if (!findDimensions()) {
    return false;
  }
  const AutoTransparentColorContainer container(this);
  createMask(&container);
  return true;
}

bool TransparentBitmap::init(ATL::_U_STRINGorID resourceId, COLORREF transparentColor) {
  if (!m_bitmap.LoadBitmap(resourceId)) {
    return false;
  }
  if (!findDimensions()) {
    return false;
  }
  const ConstTransparentColorContainer container(transparentColor);
  createMask(&container);
  return true;
}

bool TransparentBitmap::init(ATL::_U_STRINGorID resourceId, POINT transparentPixel) {
  if (!m_bitmap.LoadBitmap(resourceId)) {
    return false;
  }
  if (!findDimensions()) {
    return false;
  }
  const PointTransparentColorContainer container(transparentPixel);
  createMask(&container);
  return true;
}

bool TransparentBitmap::init(ATL::_U_STRINGorID resourceId, int width, int height) {
  if (!m_bitmap.LoadBitmap(resourceId)) {
    return false;
  }
  m_width = width;
  m_height = height;
  const AutoTransparentColorContainer container(this);
  createMask(&container);
  return true;
}

bool TransparentBitmap::init(ATL::_U_STRINGorID resourceId, int width, int height, COLORREF transparentColor) {
  if (!m_bitmap.LoadBitmap(resourceId)) {
    return false;
  }
  m_width = width;
  m_height = height;
  const ConstTransparentColorContainer container(transparentColor);
  createMask(&container);
  return true;
}

bool TransparentBitmap::init(ATL::_U_STRINGorID resourceId, int width, int height, POINT transparentPixel) {
  if (!m_bitmap.LoadBitmap(resourceId)) {
    return false;
  }
  m_width = width;
  m_height = height;
  const PointTransparentColorContainer container(transparentPixel);
  createMask(&container);
  return true;
}

#define ROP_DSTCOPY 0x00AA0029

void TransparentBitmap::draw(HDC hdc, int x, int y) const {
  CDCHandle dc(hdc);
#if 1
  CDC bitmapDC;
  bitmapDC.CreateCompatibleDC(dc);
  const HBITMAP oldBitmap = bitmapDC.SelectBitmap(m_bitmap);
  dc.MaskBlt(x, y, m_width, m_height, bitmapDC, 0, 0, m_mask, 0, 0, MAKEROP4(ROP_DSTCOPY, SRCCOPY));
  bitmapDC.SelectBitmap(oldBitmap);
#else
  {
    CDC maskDC;
    maskDC.CreateCompatibleDC(dc);
    HBITMAP oldMaskBitmap = maskDC.SelectBitmap(m_mask);
    dc.BitBlt(x, y, m_width, m_height, maskDC, 0, 0, SRCAND);
    maskDC.SelectBitmap(oldMaskBitmap);
  }
  {
    CDC bitmapDC;
    bitmapDC.CreateCompatibleDC(dc);
    const HBITMAP oldBitmap = bitmapDC.SelectBitmap(m_bitmap);
    dc.BitBlt(x, y, m_width, m_height, bitmapDC, 0, 0, SRCPAINT);
    bitmapDC.SelectBitmap(oldBitmap);
  }
#endif
}
