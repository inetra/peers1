
/*
 * ApexDC speedmod (c) SMT 2007
 */


#ifndef MESSAGES_PAGE_H
#define MESSAGES_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"


class MessagesPage : 
  public PeersPropertyPage<IDD_MESSAGES_PAGE>, 
  public PropPageImpl<MessagesPage,21>
{
private:
  TCHAR *title;
public:
        MessagesPage() {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_MESSAGES)).c_str());
                SetTitle(title);
        };
        virtual ~MessagesPage() { free(title); }

        BEGIN_MSG_MAP(MessagesPage)
                MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
        END_MSG_MAP()

        LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

        virtual void write();

protected:
        static Item items[];
        static TextItem texts[];
        static ListItem listItems[];
};

#endif //MESSAGES_PAGE_H
