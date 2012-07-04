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

#ifndef CHAT_CTRL_H
#define CHAT_CTRL_H

#include "atlstr.h"
//[-]PPA [Doxygen 1.5.1]  #include "TypedListViewCtrl.h"
#include "ImageDataObject.h"
#include "UserInfo.h"

#ifndef _RICHEDIT_VER
# define _RICHEDIT_VER 0x0300
#endif

#include <AtlCrack.h>

class UserInfo;

class ChatCtrl: public CRichEditCtrl {
protected:
        // TypedListViewCtrl<UserInfo, IDC_USERS> *m_pUsers;
        string clientUrl; // !SMT!-S
        bool isOnline(const CAtlString& aNick); // !SMT!-S

        bool m_boAutoScroll;
public:
        ChatCtrl();
        virtual ~ChatCtrl() {}

        LRESULT OnRButtonDown(POINT pt);

        bool HitNick(POINT p, CAtlString& sNick, int& piBegin , int& piEnd);
        bool HitIP(POINT p, CAtlString& sIP, int& piBegin, int& piEnd);
        bool HitURL();

        tstring LineFromPos(POINT p) const;

        void AdjustTextSize();
        void AppendText(const Identity& i, const tstring& sMyNick, const tstring& sTime, const LPCTSTR sMsg, CHARFORMAT2& cf, bool bUseEmo = true);
        void AppendTextOnly(const tstring& sMyNick, const LPCTSTR sMsg, CHARFORMAT2& cf, bool bMyMess, const tstring& sAuthor, bool isBot);

        void GoToEnd();
        bool GetAutoScroll() const { return m_boAutoScroll; }
        void SetAutoScroll(bool boAutoScroll);
        //void SetUsers(TypedListViewCtrl<UserInfo, IDC_USERS> *pUsers);
        void SetClientUrl(const string& url) { clientUrl = url; } // !SMT!-S
        void SetTextStyleMyNick(CHARFORMAT2 ts) { WinUtil::m_TextStyleMyNick = ts; };

        static tstring sSelectedLine;
        static tstring sSelectedIP;
        static tstring sTempSelectedUser;
};


#endif //!defined(AFX_CHAT_CTRL_H__595F1372_081B_11D1_890D_00A0244AB9FD__INCLUDED_)
