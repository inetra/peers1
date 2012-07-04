/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "TraceManager.h"

TraceManager::TraceManager(){
	f = new File("trace.log", File::WRITE, File::OPEN | File::CREATE);
	f->setEndPos(0);
}

void TraceManager::print(string msg) {
	time_t now = time(NULL);
	char buf[21];
	if(!strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", localtime(&now))) {
		strcpy(buf, "xxxx-xx-xx xx:xx:xx ");
	}
	const string indent = string(getIndent(GetCurrentThreadId()), ' ');
	Lock l(cs);
	try {
		f->write(buf + indent + msg + "\r\n");
	}
	catch (const FileException&) {
		// ...
	}
}

void CDECL TraceManager::trace_print(const char* format, ...) throw() {
	va_list args;
	va_start(args, format);

	char buf[512];

	_vsnprintf(buf, sizeof(buf), format, args);

	print(buf);
	va_end(args);

}

void CDECL TraceManager::trace_start(const char* format, ...) throw()
{
	va_list args;
	va_start(args, format);

	char buf[512];

	_vsnprintf(buf, sizeof(buf), format, args);

	print((string)"START " + buf);
	
	incIndent(GetCurrentThreadId());

	va_end(args);
}

void CDECL TraceManager::trace_end(const char* format, ...) throw()
{
	va_list args;
	va_start(args, format);

	char buf[512];

	_vsnprintf(buf, sizeof(buf), format, args);

	decIndent(GetCurrentThreadId());

	print((string)"END " +buf);

	va_end(args);
}


/**
 * @file
 * $Id: TraceManager.cpp,v 1.3 2008/04/30 02:22:15 alexey Exp $
 */
