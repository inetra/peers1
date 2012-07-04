#include "stdafx.h"
#include "HubFrameFactory.h"
#include "../windows/HubFrame.h"

void HubFrameFactory::openWindow(const tstring& server
								 , bool autoConnect
                                 , const tstring& rawOne
                                 , const tstring& rawTwo
                                 , const tstring& rawThree 
                                 , const tstring& rawFour
                                 , const tstring& rawFive
                                 , int windowposx, int windowposy, int windowsizex, int windowsizey, int windowtype, int chatusersplit, bool userliststate,
                                 string sColumsOrder, string sColumsWidth, string sColumsVisible)
{
  HubFrame::openWindow(server
								 , autoConnect
                                 , rawOne 
                                 , rawTwo 
                                 , rawThree 
                                 , rawFour 
                                 , rawFive 
                                 , windowposx , windowposy , windowsizex , windowsizey ,
                                 windowtype , chatusersplit , userliststate ,
                                 sColumsOrder , sColumsWidth , sColumsVisible );
}

void HubFrameFactory::closeAll(size_t thershold) {
  HubFrame::closeAll(thershold);
}

void HubFrameFactory::closeDisconnected() {
  HubFrame::closeDisconnected();
}

void HubFrameFactory::reconnectDisconnected() {
  HubFrame::reconnectDisconnected();
}

void HubFrameFactory::resortUsers() {
  HubFrame::resortUsers();
}

void HubFrameFactory::addDupeUsersToSummaryMenu(const int64_t &share, const string& ip) {
  HubFrame::addDupeUsersToSummaryMenu(share, ip);
}

bool HubFrameFactory::activateThisChat(MDIContainer::Window window) {
  HubChatFrame *hubChatFrame = dynamic_cast<HubChatFrame*>(window);
  if (hubChatFrame) return true;
  HubFrame* hubFrame = dynamic_cast<HubFrame*>(window);
  if (hubFrame) {
    hubFrame->activateChat(true);
    return true;
  }
  return false;
}

bool HubFrameFactory::activateAnyChat() {
  vector<MDIContainer::Window> windows = MDIContainer::list();
  for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
    HubChatFrame *hubChatFrame = dynamic_cast<HubChatFrame*>(*i);
    if (hubChatFrame) {
      WinUtil::MDIActivate(hubChatFrame->m_hWnd);
      return true;
    }
  }
  for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
    HubFrame* hubFrame = dynamic_cast<HubFrame*>(*i);
    if (hubFrame) {
      hubFrame->activateChat(true);
      return true;
    }
  }
  return false;
}

bool HubFrameFactory::activateHubWindow(const string& hubURL, bool activateChat) {
	vector<MDIContainer::Window> windows = MDIContainer::list();
	for (vector<MDIContainer::Window>::iterator i = windows.begin(); i != windows.end(); ++i) {
		if (activateChat) {
			HubChatFrame *hubChatFrame = dynamic_cast<HubChatFrame*>(*i);
			if (hubChatFrame) {
				if (Util::findSubString(hubChatFrame->getClient()->getHubUrl(), hubURL) != string::npos) {
					WinUtil::MDIActivate(hubChatFrame->m_hWnd);
					return true;
				}
			}
		}
		else {
			HubFrame* hubFrame = dynamic_cast<HubFrame*>(*i);
			if (hubFrame) {
				if (Util::findSubString(hubFrame->getClient()->getHubUrl(), hubURL) != string::npos) {
					WinUtil::MDIActivate(hubFrame->m_hWnd);
					return true;
				}
			}
		}
	}
	return false;
}
