/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(SPEAKER_H)
#define SPEAKER_H

#include <typeinfo>
#include "CriticalSection.h"

template<typename Listener>
class Speaker {
	typedef vector<Listener*> ListenerList;
	typedef typename ListenerList::const_iterator ListenerIter;

public:
	Speaker() throw() { }
	virtual ~Speaker() throw() { }

	template<typename T0>
	void fire(T0 type) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type);
		}
	}

	template<typename T0, class T1>
	void fire(T0 type, const T1& p1) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1);
		}
	}
	template<typename T0, class T1>
	void fire(T0 type, T1& p1) throw() {
		Lock l(listenerCS);
		tmp = listeners;
#ifdef _DEBUG_SPEAKER
                dcdebug("fire %s tmp.size=%d\n", typeid(type).name(), tmp.size() );
#endif
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1);
		}
	}

	template<typename T0, class T1, class T2>
	void fire(T0 type, const T1& p1, const T2& p2) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1, p2);
		}
	}

	template<typename T0, class T1, class T2, class T3>
	void fire(T0 type, const T1& p1, const T2& p2, const T3& p3) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1, p2, p3);
		}
	}

	template<typename T0, class T1, class T2, class T3, class T4>
	void fire(T0 type, const T1& p1, const T2& p2, const T3& p3, const T4& p4) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1, p2, p3, p4);
		}
	}

	template<typename T0, class T1, class T2, class T3, class T4, class T5>
	void fire(T0 type, const T1& p1, const T2& p2, const T3& p3, const T4& p4, const T5& p5) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1, p2, p3, p4, p5);
		}
	}

	template<typename T0, class T1, class T2, class T3, class T4, class T5, class T6>
	void fire(T0 type, const T1& p1, const T2& p2, const T3& p3, const T4& p4, const T5& p5, const T6& p6) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1, p2, p3, p4, p5, p6);
		}
	}

	template<typename T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
	void fire(T0 type, const T1& p1, const T2& p2, const T3& p3, const T4& p4, const T5& p5, const T6& p6, const T7& p7) throw() {
		Lock l(listenerCS);
		tmp = listeners;
		for(ListenerIter i=tmp.begin(); i != tmp.end(); ++i ) {
			(*i)->on(type, p1, p2, p3, p4, p5, p6, p7);
		}
	}

        void addListener(Listener* aListener) {
          Lock l(listenerCS);
          if (find(listeners.begin(), listeners.end(), aListener) == listeners.end()) {
            listeners.push_back(aListener);
#ifdef _DEBUG_SPEAKER
            dcdebug("addListener %s -> %s (%d)\n", typeid(*aListener).name(), typeid(*this).name(), listeners.size());
#endif
          }
#ifdef _DEBUG_SPEAKER
          else {
            dcdebug("addListener duplicate %s -> %s (%d)\n", typeid(*aListener).name(), typeid(*this).name(), listeners.size());
          }
#endif
        }

        void removeListener(Listener* aListener) {
          Lock l(listenerCS);
          ListenerList::iterator it = find(listeners.begin(), listeners.end(), aListener);
          if (it != listeners.end()) {
            listeners.erase(it);
#ifdef _DEBUG_SPEAKER
            dcdebug("removeListener %s -> %s (%d)\n", typeid(*aListener).name(), typeid(*this).name(), listeners.size());
#endif
          }
#ifdef _DEBUG_SPEAKER
          else {
            dcdebug("remove not registered listener %s (%d remained)\n", typeid(*aListener).name(), listeners.size());
          }
#endif
        }

	void removeListeners() {
		Lock l(listenerCS);
		listeners.clear();
	}
	
protected:
	ListenerList listeners;
	ListenerList tmp;
	CriticalSection listenerCS;
};

#endif // !defined(SPEAKER_H)

/**
 * @file
 * $Id: Speaker.h,v 1.4 2008/03/10 13:35:16 alexey Exp $
 */
