#include "stdafx.h"
#include "SearchFrmFactory.h"
#include "../windows/SearchFrm.h"
#ifdef _DEBUG
#include "HubMessageControl.h"
#endif

class SearchFrameWindowListener : public WindowListener {
private:
  friend class SearchFrameFactory;
  typedef map<HWND, SearchFrame*> FrameMap;
  typedef FrameMap::const_iterator FrameIter;
  typedef pair<HWND, SearchFrame*> FramePair;
  FrameMap frames;

  inline void dumpSearchFrameAmount() {
#ifdef _DEBUG
    HubMessageControl::getInstance()->addMessage(_T("Search"), Util::emptyStringT, Text::toT(Util::toString(frames.size())));
#endif
  }

  void registerSearchFrame(SearchFrame* frame) {
    frames.insert(FramePair(frame->m_hWnd, frame));
    frame->addWindowListener(this);
    dumpSearchFrameAmount();
  }

  void closeAll() {
    for (FrameIter i = frames.begin(); i != frames.end(); ++i) {
      ::PostMessage(i->first, WM_CLOSE, 0, 0);
    }
    dumpSearchFrameAmount();
  }

  virtual void windowClosed(MDIContainer::Window window) {
    frames.erase(window->m_hWnd);
    dumpSearchFrameAmount();
  }
};

SearchFrameWindowListener listener;

void SearchFrameFactory::openWindow(const tstring& str) {
  SearchFrameFactory::Request request;
  request.str = str;
  openWindow(request);
}

void SearchFrameFactory::searchTTH(const tstring& tth) {
  SearchFrameFactory::Request request;
  request.type = SearchManager::TYPE_TTH;
  request.str = tth;
  openWindow(request);
}

void SearchFrameFactory::openWindow(const Request& request) {
  SearchFrame* pChild = new SearchFrame();
  pChild->initialize(request);
  pChild->CreateEx(WinUtil::mdiClient, pChild->rcDefault, CTSTRING(SEARCH));
  listener.registerSearchFrame(pChild);
}

void SearchFrameFactory::closeAll() {
  listener.closeAll();
}
