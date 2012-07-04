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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include <process.h>
#include "Thread.h"

#include "ResourceManager.h"

#ifndef _WIN32
pthread_mutex_t Thread::mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef _WIN32

DWORD Thread::m_threadId;

void Thread::start() throw(ThreadException) {
	join();
	if( (threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &starter, this, 0, reinterpret_cast<unsigned int*>(&m_threadId)))) == NULL) {
		throw ThreadException(STRING(UNABLE_TO_CREATE_THREAD));
	}
}

#else
void Thread::start() throw(ThreadException) {
	join();
	if(pthread_create(&threadHandle, NULL, &starter, this) != 0) {
		throw ThreadException(STRING(UNABLE_TO_CREATE_THREAD));
	}
}
#endif

/**
 * @file
 * $Id: Thread.cpp,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
