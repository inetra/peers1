//////////////////////////////////////////////////////////////////////////////////////
//
// Written by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
// For companies(Austin,TX): If you would like to get my resume, send an email.
//
// The source is free, but if you want to use it, mention my name and e-mail address
//
// History:
//    1.0      Initial version                  Zoltan Csizmadia
//
//////////////////////////////////////////////////////////////////////////////////////
//
// ExtendedTrace.h
//

#ifndef EXTENDEDTRACE_H_INCLUDED
#define EXTENDEDTRACE_H_INCLUDED
#pragma once

#include <windows.h>
#include <tchar.h>
#pragma comment( lib, "dbghelp.lib" )

BOOL InitSymInfo(PCTSTR);
BOOL UninitSymInfo();
tstring StackTrace(HANDLE, const CONTEXT* context);

#endif

/**
 * @file
 * $Id: ExtendedTrace.h,v 1.2 2008/05/03 11:17:30 alexey Exp $
 */
