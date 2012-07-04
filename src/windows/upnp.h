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

#if !defined(UPNP_H)
#define UPNP_H

#ifdef PPA_INCLUDE_UPNP

#define STATICLIB
#include "../miniupnpc-1.6/miniupnpc.h"
#include "../miniupnpc-1.6/miniupnpctypes.h"
#include "../miniupnpc-1.6/upnpcommands.h"
#include "../miniupnpc-1.6/upnperrors.h"
#undef STATICLIB

class UPnP
{
public:
	UPnP( const string, const string, const string, const unsigned short );
	~UPnP();
	HRESULT OpenPorts();
	HRESULT ClosePorts();
	string GetExternalIP();
private:
	struct UPNPDev* devlist;
	struct UPNPUrls urls;
	char lanaddr[64];
	struct IGDdatas data;

	string port;
	string description;
	string proto;
};

#endif
#endif // UPNP_H

/**
 * @file
 * $Id: upnp.h,v 1.1.1.1 2007/09/27 13:21:35 alexey Exp $
 */
