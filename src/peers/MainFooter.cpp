#include "stdafx.h"
#include "MainFooter.h"
#include "../windows/MainFrm.h"

LRESULT MainFooter::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  RECT rect;
  GetClientRect(&rect);
  SetSplitterRect(&rect);
  return 0;
}

bool MainFooter::SetSplitterPos(int xyPos, bool bUpdate)
{
  splitterResizeActive = true;
  if (m_transferView && m_hubMessages) {
    const int cxyMax = m_rcSplitter.bottom - m_rcSplitter.top;
    const int cxyRange = cxyMax - m_cxySplitBar - m_cxyBarEdge - m_hubMessages->getPreferredHeight();
    if (xyPos > cxyRange) {
      xyPos = cxyRange;
    }
    else if (xyPos < m_transferView->getHeaderHeight() && !m_transferView->isVisible()) {
      MainFrame* mainFrame = MainFrame::getMainFrame();
      if (!mainFrame->splitterResizeActive) {
        mainFrame->SetSplitterPos(mainFrame->m_xySplitterPos + (xyPos - m_transferView->getHeaderHeight()));
      }
      xyPos = m_transferView->getHeaderHeight();
    }
    else if (xyPos < 2 * m_transferView->getHeaderHeight()) {
      xyPos = m_transferView->getHeaderHeight();
    }
  }
  const bool result = splitterBase::SetSplitterPos(xyPos, bUpdate);
  splitterResizeActive = false;
  return result;
}
