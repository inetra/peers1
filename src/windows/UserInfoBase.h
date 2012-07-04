
/*
 * ApexDC speedmod (c) SMT 2007
 */

#if !defined(USER_INFO_BASE_H)
#define USER_INFO_BASE_H

class UserInfoBase {
public:
        UserInfoBase(const UserPtr& u) : user(u) { }

        void getList();
        void browseList();
        void getUserResponses();
        void checkList();
        void doReport();
        void matchQueue();
        void pm();
        void pm_msg(void *param); // !SMT!-S
        void addSummary(); // !SMT!-UI
        void grantSlotPeriod(void *period); // !SMT!-UI
        static const tstring getBroadcastPrivateMessage(); // !SMT!-S
        static uint32_t inputSlotTime(); // !SMT!-UI
        void grant();
        void grantSlotHour();
        void grantSlotDay();
        void grantSlotWeek();
        void ungrantSlot();
        void addFav();
        void removeAll();

        UserPtr& getUser() { return user; }
        const UserPtr& getUser() const { return user; }
        UserPtr user;
};

template<class T>
class UserInfoBaseHandler {
public:
        UserInfoBaseHandler() {
                aSelectedUser = NULL;
        }

        BEGIN_MSG_MAP(UserInfoBaseHandler)
                COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
                COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
                COMMAND_ID_HANDLER(IDC_CHECKLIST, onCheckList)
                COMMAND_ID_HANDLER(IDC_GET_USER_RESPONSES, onGetUserResponses)
                COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
                COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPrivateMessage)
                COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrantSlot)
                COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_HOUR, onGrantSlotHour)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_DAY, onGrantSlotDay)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_WEEK, onGrantSlotWeek)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_PERIOD, onGrantSlotPeriod) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onUnGrantSlot)
                COMMAND_ID_HANDLER(IDC_REPORT, onReport)
        END_MSG_MAP()

        LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::matchQueue);
                return 0;
        }
        LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::getList);
                return 0;
        }
        LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                ((T*)this)->getUserList().forEachSelected(&UserInfoBase::browseList);
                return 0;
        }
        LRESULT onReport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::doReport);
                return 0;
        }
        LRESULT onGetUserResponses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::getUserResponses);
                return 0;
        }
        LRESULT onCheckList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::checkList);
                return 0;
        }
        LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::addFav);
                return 0;
        }
        LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                if(aSelectedUser) {
                        (UserInfoBase(aSelectedUser).pm)();
                } else {
                        // !SMT!-S
                        if (((T*)this)->getUserList().getSelectedCount() > 1) {
                           const tstring pmessage = UserInfoBase::getBroadcastPrivateMessage();
                           ((T*)this)->getUserList().forEachSelectedParam(&UserInfoBase::pm_msg, (void*)&pmessage);
                        } else
                           ((T*)this)->getUserList().forEachSelected(&UserInfoBase::pm);
                        // !SMT!-S end
                }
                return 0;
        }
        LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::grant);
                return 0;
        }
        LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
                doAction(&UserInfoBase::removeAll);
                return 0;
        }
        LRESULT onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
                doAction(&UserInfoBase::grantSlotHour);
                return 0;
        }
        LRESULT onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
                doAction(&UserInfoBase::grantSlotDay);
                return 0;
        }
        LRESULT onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
                doAction(&UserInfoBase::grantSlotWeek);
                return 0;
        }
        LRESULT onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
                doAction(&UserInfoBase::ungrantSlot);
                return 0;
        }
        LRESULT onGrantSlotPeriod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { // !SMT!-UI
                uint32_t slotTime = UserInfoBase::inputSlotTime();
                if(aSelectedUser) {
                        (UserInfoBase(aSelectedUser).grantSlotPeriod)((void*)slotTime);
                } else {
                        ((T*)this)->getUserList().forEachSelectedParam(&UserInfoBase::grantSlotPeriod, (void*)slotTime);
                }
                return 0;
        }

        struct ADCOnly {
                ADCOnly() : adcOnly(true) { }
                void operator()(UserInfoBase* ui) { if(ui->getUser()->isSet(User::NMDC)) adcOnly = false; }

                bool adcOnly;
        };
        void checkAdcItems(CMenu& menu) {

                if(((T*)this)->getUserList().forEachSelectedT(ADCOnly()).adcOnly) {
                        menu.EnableMenuItem(IDC_BROWSELIST, MFS_ENABLED);
                } else {
                        menu.EnableMenuItem(IDC_BROWSELIST, MFS_DISABLED);
                }
        }

        void appendUserItems(CMenu& menu, bool UsersFrm = false) {
                menu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
                menu.AppendMenu(MF_STRING, IDC_BROWSELIST, CTSTRING(BROWSE_FILE_LIST));
                menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
                menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
                if(!UsersFrm) menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
                menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
                menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::userSummaryMenu, CTSTRING(USER_SUMMARY)); // !SMT!-UI
                if(UsersFrm) menu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT_FAVUSER_HUB));
                menu.AppendMenu(MF_SEPARATOR);
                menu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_FROM_ALL));
        }

        // !SMT!-UI
        void updateSummary() {
                WinUtil::clearSummaryMenu();
                doAction(&UserInfoBase::addSummary);
        }

protected:
        UserPtr aSelectedUser;

        // !SMT!-S
        void doAction(void (UserInfoBase::*func)()) {
                if (aSelectedUser) {
                        (UserInfoBase(aSelectedUser).*func)();
                } else {
                        ((T*)this)->getUserList().forEachSelected(func);
                }
        }
};

#endif // USER_INFO_BASE_H
