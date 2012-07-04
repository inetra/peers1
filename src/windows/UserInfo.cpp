/* 
 * 
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
#include "UserInfo.h"

int UserInfo::_compareItems(const UserInfo* a, const UserInfo* b, int col)  {
	if (a == NULL || b == NULL) return NULL;
	if (col == COLUMN_NICK) {
		const bool a_isOp = a->getIdentity().isOp();
		const bool b_isOp = b->getIdentity().isOp();
		if (a_isOp && !b_isOp)
			return -1;
		if (!a_isOp && b_isOp)
			return 1;
		if (BOOLSETTING(SORT_FAVUSERS_FIRST)) {
			const bool a_isFav = FavoriteManager::getInstance()->isFavoriteUser(a->getIdentity().getUser());
			const bool b_isFav = FavoriteManager::getInstance()->isFavoriteUser(b->getIdentity().getUser());
			if (a_isFav && !b_isFav)
				return -1;
			if (!a_isFav && b_isFav)
				return 1;
		}
	}
	switch(col) 
	{
	case COLUMN_SHARED:
	case COLUMN_EXACT_SHARED: return compare(a->identity.getBytesShared(), b->identity.getBytesShared());
	case COLUMN_SLOTS: return compare(Util::toInt(a->identity.get("SL")), Util::toInt(b->identity.get("SL")));
	case COLUMN_HUBS: return compare(Util::toInt(a->identity.get("HN"))+Util::toInt(a->identity.get("HR"))+Util::toInt(a->identity.get("HO")), Util::toInt(b->identity.get("HN"))+Util::toInt(b->identity.get("HR"))+Util::toInt(b->identity.get("HO")));
	default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}  
}

void UserInfo::update(const Identity& identity) {
#ifdef PPA_INCLUDE_DEAD_CODE

	if (identity.getUser()->getLastDownloadSpeed() > 0) {
		setText(COLUMN_UPLOAD_SPEED, Util::formatBytesW(identity.getUser()->getLastDownloadSpeed()) + _T("/s"));
	} else if(identity.getUser()->isSet(User::FIREBALL)) {
		setText(COLUMN_UPLOAD_SPEED, _T(">= 100 kB/s"));
	} else {
		setText(COLUMN_UPLOAD_SPEED, _T("N/A"));
	}

#endif
	const string l_hn = identity.get("HN");
	const string l_hr = identity.get("HR");
    const string l_ho = identity.get("HO");
	const tstring hn = Text::toT(l_hn);
	const tstring hr = Text::toT(l_hr);
    const tstring ho = Text::toT(l_ho);
    const tstring count = Util::toStringW(Util::toInt(l_hn)+Util::toInt(l_hr)+Util::toInt(l_ho));

	setText(COLUMN_NICK, Text::toT(identity.getNick()));
	setText(COLUMN_SHARED, Util::formatBytesW(identity.getBytesShared()));
    setText(COLUMN_EXACT_SHARED, Util::formatExactSize(identity.getBytesShared()));
	setText(COLUMN_DESCRIPTION, Text::toT(identity.getDescription()));
	setText(COLUMN_TAG, Text::toT(identity.getTag()));
	setText(COLUMN_EMAIL, Text::toT(identity.getEmail()));
#ifdef PPA_INCLUDE_DEAD_CODE
	setText(COLUMN_CONNECTION, Text::toT(identity.getConnection()));
	setText(COLUMN_VERSION, Text::toT(identity.get("CT").empty() ? identity.get("VE") : identity.get("CT")));
	setText(COLUMN_MODE, identity.isTcpActive() ? _T("Актив") : _T("Пассив"));
#endif
	setText(COLUMN_HUBS, (hn.empty() && hr.empty() && ho.empty()) ? Util::emptyStringT : (count + _T(" (") + hn + _T("/") + hr + _T("/") + ho + _T(")")));
	setText(COLUMN_SLOTS, Text::toT(identity.get("SL")));
 	string ip = identity.getIp();
#ifdef PPA_INCLUDE_DNS
    setText(COLUMN_DNS, Text::toT(Socket::nslookup(ip)));
#endif
	string country = ip.empty() ? Util::emptyString : Util::getIpCountry(ip);
	if (!country.empty())
	{
        const string::size_type comma = country.find(',');
        if (comma != string::npos)
            country = country.substr(comma+1);
		ip = country + " (" + ip + ")";
	}
 	setText(COLUMN_IP, Text::toT(ip));
#ifdef PPA_INCLUDE_DEAD_CODE
	setText(COLUMN_PK, Text::toT(identity.get("PK")));
#endif
	setIdentity(identity);
}

int UserInfoNickComparator::compare(const UserInfo* a, const UserInfo* b) {
	const bool a_isOp = a->getIdentity().isOp();
	const bool b_isOp = b->getIdentity().isOp();
	if (a_isOp && !b_isOp)
		return -1;
	if (!a_isOp && b_isOp)
		return 1;
	return Util::stricmp(a->getIdentity().getNick(), b->getIdentity().getNick());
}

int UserInfoShareComparator::compare(const UserInfo* a, const UserInfo* b) {
	int64_t sharedA = a->getIdentity().getBytesShared();
	int64_t sharedB = b->getIdentity().getBytesShared();
	if (sharedA < sharedB) {
		return -1;
	}
	else if (sharedA > sharedB) {
		return +1;
	}
	return Util::stricmp(a->getIdentity().getNick(), b->getIdentity().getNick());
}
