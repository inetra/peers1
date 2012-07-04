#pragma once
#include "../windows/TransferView.h"
#include "HubMessageControl.h"

class MainFooter : public CWindowImpl<MainFooter>, public CSplitterImpl<MainFooter, false> {
private:
  friend class MainFrame;
  typedef public CSplitterImpl<MainFooter, false> splitterBase;
  TransferView* m_transferView;
  HubMessageControl* m_hubMessages;
  bool splitterResizeActive;
public:
  MainFooter(): m_transferView(NULL), m_hubMessages(NULL), splitterResizeActive(false) {
    SetSplitterExtendedStyle(SPLIT_BOTTOMALIGNED);
  }

  HWND Create(HWND hWndParent) {
    return CWindowImpl<MainFooter>::Create(hWndParent, NULL, NULL, WS_VISIBLE | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
  }

  void setTransferView(TransferView* transferView) {
    m_transferView = transferView;
  }

  void setHubMessageControl(HubMessageControl* hubMessages) {
    m_hubMessages = hubMessages;
  }

  bool SetSplitterPos(int xyPos = -1, bool bUpdate = true);

  BEGIN_MSG_MAP(MainFooter)
    MESSAGE_HANDLER(WM_SIZE, onSize);
    CHAIN_MSG_MAP(splitterBase);
  END_MSG_MAP();

  LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

};
