#pragma once
#include <atlgdi.h>

class TransparentColorContainer {
public:
  virtual COLORREF getTransparentColor(CDCHandle dc) const = 0;
};

class TransparentBitmap {
private:
  int m_width;
  int m_height;
  CBitmap m_bitmap;
  CBitmap m_mask;
  void createMask(const TransparentColorContainer* container);
  bool findDimensions();
public:
  TransparentBitmap();
  bool init(ATL::_U_STRINGorID resourceId);
  bool init(ATL::_U_STRINGorID resourceId, COLORREF transparentColor);
  bool init(ATL::_U_STRINGorID resourceId, POINT transparentPixel);
  bool init(ATL::_U_STRINGorID resourceId, int width, int height);
  bool init(ATL::_U_STRINGorID resourceId, int width, int height, COLORREF transparentColor);
  bool init(ATL::_U_STRINGorID resourceId, int width, int height, POINT transparentPixel);

  void draw(HDC hdc, int x, int y) const;

  int getHeight() const { return m_height; }
  int getWidth() const { return m_width; }

  operator HBITMAP () const { return m_bitmap; }
};
