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

#include "stdafx.h"
#ifdef PPA_INCLUDE_UPNP

#include "UPnP.h"
#include <atlconv.h>
#include <sstream>

UPnP::UPnP(const string theIPAddress, const string theProtocol, const string theDescription, const unsigned short thePort):
 proto(theProtocol), description(theDescription) {
	 std::stringstream ss;
	 ss << thePort;
	 port = ss.str();
	int error;
	devlist = upnpDiscover(500, 0, 0, 0, 0, &error);
	if (devlist)
	{
		error = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
		if (error != 1) {
			freeUPNPDevlist(devlist);
			devlist = NULL;
			if (error > 0) {
				FreeUPNPUrls(&urls);
			}
		}
	}
}


// Opens the UPnP ports defined when the object was created
HRESULT UPnP::OpenPorts() {
	if (!devlist) return E_FAIL;
	UNSIGNED_INTEGER result = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.c_str(), port.c_str(), lanaddr, description.c_str(), proto.c_str(), 0, "0");
	return (result == UPNPCOMMAND_SUCCESS)?S_OK:E_FAIL;
}

// Closes the UPnP ports defined when the object was created
HRESULT UPnP::ClosePorts() {
	if (!devlist) return E_FAIL;
	UNSIGNED_INTEGER result = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.c_str(), proto.c_str(), 0);
	return (result == UPNPCOMMAND_SUCCESS)?S_OK:E_FAIL;
}

// Returns the current external IP address
string UPnP::GetExternalIP() {
	return std::string();
}

UPnP::~UPnP() {
	if (devlist)
	{
		FreeUPNPUrls(&urls);
		freeUPNPDevlist(devlist);
	}
}
#endif

/**
 * @file
 * $Id: upnp.cpp,v 1.2 2008/03/10 06:57:19 alexey Exp $
 */
