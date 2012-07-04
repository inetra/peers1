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


#if !defined(DCPLUSPLUS_WIN32_STDAFX_H)
#define DCPLUSPLUS_WIN32_STDAFX_H



#include "../client/stdinc.h"
#include "../client/ResourceManager.h"
#include "../client/AssocVector.h" //[+]PPA

enum {
  // для выполнения действий в GUI потоке
  WM_SPEAKER  = WM_APP + 500,
  // для выполнения обработки когда будет закончено создание окна
  WPM_AFTER_CREATE,
  // сообщение отправляемое через значок в трее
  WPM_TRAY,
  // сообщение, отправляемое активному дочернему окну, чтобы обновить состояние кнопок
  WPM_IDLE,
  // сообщение, отправляемое дочерним окнам при минимизации главного окна приложения
  WPM_APP_MINIMIZE,
  // сообщение, отправляемое активному дочернему окну при активизации главного окна приложения
  WPM_APP_ACTIVATE,
  // сообщение, отправляемое дочерним MDI окнам для получения предпочтительных размеров
  WPM_MDI_CHILD_GETMINMAXINFO
};

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WTL_NO_CSTRING
#define _ATL_NO_OPENGL
#define _ATL_NO_MSIMG
#define _ATL_NO_COM
#define _ATL_NO_OLD_NAMES
#define _ATL_NO_COM_SUPPORT
#define _ATL_NO_PERF_SUPPORT
#define _ATL_NO_SERVICE

#include <winsock2.h>

// Fix nt4 startup
#include <multimon.h>

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#define _WTL_MDIWINDOWMENU_TEXT CTSTRING(MENU_WINDOW)
#define _WTL_CMDBAR_VISTA_MENUS 0
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlmisc.h>
#include <atlsplit.h>
#include <Shellapi.h>
#endif // _WIN32

#define GREEN_COLOR RGB(0,130,0)
#define HOVER_COLOR RGB(222,235,247)
#define HEX_RGB(color) (((color & 0xFF0000) >> 16) | (color & 0x00FF00) | ((color & 0x0000FF) << 16))
#define PPA_INCLUDE_UPNP
#define PPA_INCLUDE_CHECK_UPDATE
#define MAGNET_DIALOG

// want M_PI from <math.h>
#define _USE_MATH_DEFINES

// it should be included before DCPlusPlus.h
#include "../peers/IniFile.h"

#include "../client/DCPlusPlus.h"
#include "../client/Util.h"
#include "../client/File.h"
#include "../client/SimpleXML.h"
#include "../client/LogManager.h"
#include "../client/StringTokenizer.h"
#include "../client/SettingsManager.h"
#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/ResourceManager.h"
#include "resource.h"
#include "WinUtil.h"
#include "FlatTabCtrl.h"
#include "StaticFrame.h"
#include "../peers/ChildMinMaxInfo.h"
#include "../peers/PeersVersion.h"
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(DCPLUSPLUS_WIN32_STDAFX_H)

/**
 * @file
 * $Id: stdafx.h,v 1.12.2.1 2008/10/12 12:37:47 alexey Exp $
 */
