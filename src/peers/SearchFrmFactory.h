#pragma once
#include "../client/SearchManager.h"

class SearchFrameFactory {
private:
  friend class SearchFrame;
  void static windowClosed(HWND hWnd);
public:
  struct Request {
    tstring str;
    LONGLONG size;
    SearchManager::SizeModes mode;
    SearchManager::TypeModes type;
    Request():size(0), mode(SearchManager::SIZE_DONTCARE), type(SearchManager::TYPE_ANY) {
    }

  };
  static void openWindow(const Request& request);
  static void searchTTH(const tstring& tth);
  static void openWindow(const tstring& searchString);
  static void closeAll();
};
