#pragma once

class CImageListContainer : public CImageList {
private:
  BOOL Destroy() {
    return CImageList::Destroy();
  }
public:
  ~CImageListContainer() {
    Destroy();
  }
};
