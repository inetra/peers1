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

#if !defined(SINGLETON_H)
#define SINGLETON_H

template<typename T>
class Singleton {
public:
	Singleton() { }
	virtual ~Singleton() { }

	static T* getInstance() {
		return instance;
	}
	
	static void newInstance() {
		if(instance)
			delete instance;
		
		instance = new T();
	}
	
	static void deleteInstance() {
		if(instance)
			delete instance;
		instance = NULL;
	}
protected:
	static T* instance;
private:
	Singleton(const Singleton&);
	Singleton& operator=(const Singleton&);

};

template<class T> T* Singleton<T>::instance = NULL;

template<typename T, typename P>
class ParametrizableSingleton {
public:
	ParametrizableSingleton() { }
	virtual ~ParametrizableSingleton() { }

	static T* getInstance() {
		return instance;
	}
	
	static void newInstance(const P& p) {
		if(instance)
			delete instance;
		
		instance = new T(p);
	}
	
	static void deleteInstance() {
		if(instance)
			delete instance;
		instance = NULL;
	}
protected:
	static T* instance;
private:
	ParametrizableSingleton(const ParametrizableSingleton&);
	ParametrizableSingleton& operator=(const ParametrizableSingleton&);

};

template<class T,class P> T* ParametrizableSingleton<T,P>::instance = NULL;

#endif // !defined(SINGLETON_H)

/**
 * @file
 * $Id: Singleton.h,v 1.2 2007/10/15 10:54:30 alexey Exp $
 */
