#include "stdafx.h"
#include "PrivateFrameFactory.h"
#include "../windows/ChatBot.h"
#include "../windows/PrivateFrame.h"
#ifdef _DEBUG
#include "HubMessageControl.h"
#endif
#include "Sounds.h"

typedef HASH_MAP<UserPtr, PrivateFrame*, User::HashFunction> FrameMap;
typedef FrameMap::iterator FrameIter;

class PrivateFrameWindowListener : public WindowListener {
private:
  friend class PrivateFrameFactory;
  FrameMap frames;

  void add(const UserPtr& user, PrivateFrame* frame) {
    frames[user] = frame;
    frame->addWindowListener(this);
#ifdef _DEBUG
    HubMessageControl::getInstance()->addMessage(_T("PrivateFrame"), Util::emptyStringT, Util::toStringW(frames.size()));
#endif
  }

  virtual void windowClosed(MDIContainer::Window window) {
    for (FrameIter i = frames.begin(); i != frames.end(); ++i) {
      if (i->second == window) {
        frames.erase(i);
        break;
      }
    }
#ifdef _DEBUG
    HubMessageControl::getInstance()->addMessage(_T("PrivateFrame"), Util::emptyStringT, Util::toStringW(frames.size()));
#endif
  }
};

static PrivateFrameWindowListener listener;

void PrivateFrameFactory::gotMessage(Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, bool annoying) { // !SMT!-S
	PrivateFrame* p = NULL;
	bool myPM = replyTo == ClientManager::getInstance()->getMe();
	const UserPtr& user = myPM ? to : replyTo;
	
	FrameIter i = listener.frames.find(user);
	if(i == listener.frames.end()) {
                if(!annoying || listener.frames.size() > 200) return; // !SMT!-S
		p = new PrivateFrame(user);
		listener.add(user, p);
		p->addLine(from, aMessage);
		if(Util::getAway()) {
		// Again, is there better way for this?
		FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID())));
			if(!(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && user->isSet(User::BOT)))
				if(fhe) {
					if(!fhe->getAwayMsg().empty())
						p->sendMessage(Text::toT(fhe->getAwayMsg()));
					else
						p->sendMessage(Text::toT(Util::getAwayMessage()));
				} else {
					p->sendMessage(Text::toT(Util::getAwayMessage()));
				}
		}

		if(BOOLSETTING(POPUP_NEW_PM)) {
			if(BOOLSETTING(PM_PREVIEW)) {
                                tstring message = aMessage.substr(0, 250);
                                WinUtil::ShowBalloonTip(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
			} else {
				pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo);
				WinUtil::ShowBalloonTip((Text::toT(replyTo->getFirstNick() + " - ") + hubs.first).c_str(), CTSTRING(PRIVATE_MESSAGE));
			}
		}

		if(BOOLSETTING(PRIVATE_MESSAGE_BEEP) || BOOLSETTING(PRIVATE_MESSAGE_BEEP_OPEN)) {
			Sounds::PlaySound(SettingsManager::BEEPFILE, true);
		}

                if (!myPM) ChatBot::getInstance()->onMessage(user, aMessage, true); // !SMT!-CB
	} else {
		if(!myPM) {
			if(BOOLSETTING(POPUP_PM)) {
				if(BOOLSETTING(PM_PREVIEW)) {
                                        tstring message = aMessage.substr(0, 250);
                                        WinUtil::ShowBalloonTip(message.c_str(), CTSTRING(PRIVATE_MESSAGE));
				} else {
					pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo);
					WinUtil::ShowBalloonTip((Text::toT(replyTo->getFirstNick() + " - ") + hubs.first).c_str(), CTSTRING(PRIVATE_MESSAGE));
				}
			}

			if(BOOLSETTING(PRIVATE_MESSAGE_BEEP)) {
				Sounds::PlaySound(SettingsManager::BEEPFILE, true);
			}
                        ChatBot::getInstance()->onMessage(user, aMessage, false); // !SMT!-CB
		}
		i->second->addLine(from, aMessage);
	}
}

void PrivateFrameFactory::openWindow(const UserPtr& replyTo, const tstring& msg) {
	PrivateFrame* p = NULL;
	FrameIter i = listener.frames.find(replyTo);
	if(i == listener.frames.end()) {
		if(listener.frames.size() > 200) return;
		p = new PrivateFrame(replyTo);
		listener.add(replyTo, p);
		p->CreateEx(WinUtil::mdiClient);
	} else {
		p = i->second;
		if(::IsIconic(p->m_hWnd))
			::ShowWindow(p->m_hWnd, SW_RESTORE);
		p->MDIActivate(p->m_hWnd);
	}
	if(!msg.empty())
		p->sendMessage(msg);
}

bool PrivateFrameFactory::isOpen(const UserPtr& u) { 
  return listener.frames.find(u) != listener.frames.end(); 
}

void PrivateFrameFactory::closeAll(){
	for(FrameIter i = listener.frames.begin(); i != listener.frames.end(); ++i)
		i->second->PostMessage(WM_CLOSE, 0, 0);
}

void PrivateFrameFactory::closeAllOffline() {
	for(FrameIter i = listener.frames.begin(); i != listener.frames.end(); ++i) {
		if(!i->first->isOnline())
			i->second->PostMessage(WM_CLOSE, 0, 0);
	}
}

// !SMT!-S
bool PrivateFrameFactory::closeUser(const UserPtr& u)
{
        FrameIter i = listener.frames.find(u);
        if (i == listener.frames.end()) return false;
        i->second->PostMessage(WM_CLOSE, 0, 0);
        return true;
}
