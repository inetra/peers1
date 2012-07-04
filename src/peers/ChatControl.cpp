#include "stdafx.h"
#include "ChatControl.h"
#include "../windows/AGEmotionSetup.h"
#include "../windows/LineDlg.h"
#include "../windows/EmoticonsDlg.h"
#include "ControlAdjuster.h"
#include "CaptionFont.h"
#include "Sounds.h"

ChatControl::ChatControl(ChatControlListener* listener):
m_listener(listener),
timeStamps(BOOLSETTING(TIME_STAMPS)),
m_headerHeight(32),
curCommandPosition(0),
currentNeedlePos(-1),
containerView(this, mapView),
containerMessage(this, mapMessage),
containerEmoticons(this, mapEmoticons)
{
  initCommandMap();
}

LRESULT ChatControl::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);
  ctrlClient.LimitText(0);
  ctrlClient.SetFont(WinUtil::font);
  ctrlClient.SetAutoURLDetect(false);
  ctrlClient.SetEventMask(ctrlClient.GetEventMask() | ENM_LINK);
  ctrlClient.SetClientUrl(m_listener->getClient()->getHubUrl()); // !SMT!-S
  ctrlClient.SetBackgroundColor(WinUtil::bgColor);
  containerView.SubclassWindow(ctrlClient);

  ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_MULTILINE, WS_EX_CLIENTEDGE);
  ctrlMessage.SetFont(WinUtil::font);
  ctrlMessage.SetLimitText(9999);
  containerMessage.SubclassWindow(ctrlMessage);

  ctrlSend.Create(m_hWnd, rcDefault, _T("Send"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_CENTER, 0, IDC_CHAT_SEND);
  ctrlSend.SetFont(WinUtil::font);

  ctrlEmoticons.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);
  ctrlEmoticons.SetBitmap((HBITMAP) ::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_EMOTICON), IMAGE_BITMAP, 0, 0, LR_SHARED));
  containerEmoticons.SubclassWindow(ctrlEmoticons);
  m_headerHeight = ControlAdjuster::adjustHeaderHeight(m_hWnd);
  m_headerIcon.init(IDB_PEERS_TOOLBAR_CHAT);
  return 0;
}

LRESULT ChatControl::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CRect r;
  GetClientRect(r);
  r.top += m_headerHeight;
  CRect temp(r);
  temp.bottom -= 50;
  ctrlClient.MoveWindow(temp);
  temp=r;
  temp.top = temp.bottom - 50;
  temp.right -= 100;
  ctrlMessage.MoveWindow(temp);
  temp=r;
  temp.left = temp.right - 90;
  temp.right -= 30;
  temp.top = temp.bottom - 40;
  ctrlSend.MoveWindow(temp);
  temp=r;
  temp.left = temp.right - 30;
  temp.top = temp.bottom - 40;
  ctrlEmoticons.MoveWindow(temp);
  return 0;
}

LRESULT ChatControl::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_BTNFACE));
  const int y = (m_headerHeight - m_headerIcon.getHeight()) / 2;
  m_headerIcon.draw(dc, y, y);
  const CaptionFont font(CaptionFont::BOLD);
  const HFONT oldFont = dc.SelectFont(font);
  const int textHeight = WinUtil::getTextHeight(dc);
  const tstring title = TSTRING(HUB_CHAT_FRAME_TITLE);  
  dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
  dc.TextOut(y + m_headerIcon.getWidth() + 2, (m_headerHeight - textHeight) / 2, title.c_str(), title.length());
  dc.SelectFont(oldFont);
  return 0;
}

extern CAGEmotionSetup* g_pEmotionsSetup;

LRESULT ChatControl::onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  TCHAR buf[256];
  emoMenu.GetMenuString(wID, buf, 256, MF_BYCOMMAND);
  if (buf != Text::toT(SETTING(EMOTICONS_FILE))) {
    SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
    delete g_pEmotionsSetup;
    g_pEmotionsSetup = NULL;
    g_pEmotionsSetup = new CAGEmotionSetup;
    if ((g_pEmotionsSetup == NULL) || (!g_pEmotionsSetup->Create())) {
      dcassert(FALSE);
      return -1;
    }
  }
  return 0;
}

void ChatControl::addCommandToHistory(const tstring& command) {
  // save command in history, reset current buffer pointer to the newest command
  curCommandPosition = prevCommands.size(); //this places it one position beyond a legal subscript  
  if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != command) {
    ++curCommandPosition;
    prevCommands.push_back(command);
  }
  currentCommand = Util::emptyStringT;
}

tstring ChatControl::findTextPopup() {
  LineDlg finddlg;
  finddlg.title = CTSTRING(SEARCH);
  finddlg.description = CTSTRING(SPECIFY_SEARCH_STRING);
  if (finddlg.DoModal() == IDOK) {
    return finddlg.line;
  }
  else {
    return Util::emptyStringT;
  }
}

void ChatControl::findText(tstring const& needle) throw() {
  int max = ctrlClient.GetWindowTextLength();
  // a new search? reset cursor to bottom
  if (needle != currentNeedle || currentNeedlePos == -1) {
    currentNeedle = needle;
    currentNeedlePos = max;
  }
  // set current selection
  FINDTEXT ft;
  ft.chrg.cpMin = currentNeedlePos;
  ft.chrg.cpMax = 0; // поиск идет в обратную сторону, поэтому параметры передаем в обратном порядке.
  ft.lpstrText = needle.c_str();
  // empty search? stop
  if (needle.empty()) {
    return;
  }
  // find upwards
  currentNeedlePos = (int) ctrlClient.SendMessage(EM_FINDTEXT, 0, (LPARAM)&ft);
  // not found? try again on full range
  if (currentNeedlePos == -1 && ft.chrg.cpMin != max) { // no need to search full range twice
    currentNeedlePos = max;
    ft.chrg.cpMin = currentNeedlePos;
    currentNeedlePos = (int)ctrlClient.SendMessage(EM_FINDTEXT, 0, (LPARAM)&ft);
  }
  // found? set selection
  if (currentNeedlePos != -1) {
    ft.chrg.cpMin = currentNeedlePos;
    ft.chrg.cpMax = currentNeedlePos + needle.length();
    ctrlClient.SetFocus();
    ctrlClient.SendMessage(EM_EXSETSEL, 0, (LPARAM)&ft);
  } 
  else {
    addSystemMessage(CTSTRING(STRING_NOT_FOUND) + needle);
    currentNeedle = Util::emptyStringT;
  }
}

LRESULT ChatControl::onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  EmoticonsDlg dlg;
  ctrlEmoticons.GetWindowRect(dlg.pos);
  dlg.DoModal(m_hWnd);
  if (!dlg.result.empty()) {
    int start, end;
    ctrlMessage.GetSel(start, end);
    const int len = ctrlMessage.GetWindowTextLength();
    AutoArray<TCHAR> message(len + 1);
    ctrlMessage.GetWindowText(message, len + 1);
    ctrlMessage.SetWindowText((tstring(message, start) + dlg.result + tstring(message + end, len - end)).c_str());
    ctrlMessage.SetFocus();
    start += dlg.result.length();
    ctrlMessage.SetSel(start, start);
  }
  return 0;
}

LRESULT ChatControl::onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
  ENLINK* pEL = (ENLINK*) pnmh;

  if (pEL->msg == WM_LBUTTONUP) {
    long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
    AutoArray<TCHAR> sURLTemp(lEnd - lBegin + 1);
    ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
    tstring sURL = sURLTemp;
    WinUtil::openLink(sURL);
  } 
  else if (pEL->msg == WM_RBUTTONUP) {
    long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
    AutoArray<TCHAR> sURLTemp(lEnd - lBegin +1);
    ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
    sSelectedURL = sURLTemp;

    ctrlClient.SetSel(lBegin, lEnd);
    ctrlClient.InvalidateRect(NULL);
    return 0;
  }
  return 0;
}

LRESULT ChatControl::onSend(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  onSend();
  return 0;
}

void ChatControl::onSend() {
  const int messageLength = ctrlMessage.GetWindowTextLength();
  if (messageLength > 0) {
    AutoArray<TCHAR> msg(messageLength + 1);
    ctrlMessage.GetWindowText(msg, messageLength + 1);
    tstring s(msg, messageLength);
    addCommandToHistory(s);
    if (ChatCommandContext::isCommand(s)) {
      ChatCommandContext context(s);
      processCommand(context);
      if (context.clearMessageWindow) {
        ctrlMessage.SetWindowText(_T(""));
      }
    }
    else {
      m_listener->chatSendMessage(s);
      ctrlMessage.SetWindowText(_T(""));
    }
  }
  else {
    MessageBeep(MB_ICONEXCLAMATION);
  }
}

void ChatControl::processCommand(ChatCommandContext& context) {
  ChatCommandMap::iterator result = m_commands.find(Text::fromT(context.command));
  if (result != m_commands.end()) {
    (this->*(result->second))(&context);
  }
  else {
    if (!m_listener->chatExecuteCommand(&context)) {
      addSystemMessage(TSTRING(UNKNOWN_COMMAND) + context.command);
    }
  }
}

void ChatControl::addSystemMessage(const tstring& aLine) {
  addLine(Identity(NULL, 0), Util::emptyString, Util::emptyString, _T("*** ") + aLine, WinUtil::m_ChatTextSystem, false);
}

ChatControl::ChatCommandMap ChatControl::m_commands;

void ChatControl::initCommandMap() {
  if (!m_commands.empty()) {
    return;
  }
  m_commands["c"] = &ChatControl::commandClear;
  m_commands["cls"] = &ChatControl::commandClear;
  m_commands["clear"] = &ChatControl::commandClear;
  m_commands["find"] = &ChatControl::commandFind;
  m_commands["ts"] = &ChatControl::commandToggleTimestamps;
}

void ChatControl::commandClear(ChatCommandContext*) {
  ctrlClient.SetWindowText(_T(""));
}

void ChatControl::commandFind(ChatCommandContext* context) {
  if (context->param.empty()) {
    context->param = findTextPopup();
  }
  if (!context->param.empty()) {
    findText(context->param);
  }
}

void ChatControl::commandToggleTimestamps(ChatCommandContext*) {
  timeStamps = !timeStamps;
  addSystemMessage(timeStamps ? TSTRING(TIMESTAMPS_ENABLED) : TSTRING(TIMESTAMPS_DISABLED));
}

LRESULT ChatControl::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  HWND focus = GetFocus();
  if (focus == ctrlClient.m_hWnd) {
    ctrlClient.Copy();
  }
  return 0;
}

LRESULT ChatControl::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  HWND focus = GetFocus();
  if (focus == ctrlClient.m_hWnd) {
    ctrlClient.SetSelAll();
  }
  return 0;
}

LRESULT ChatControl::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  HWND focus = GetFocus();
  if (focus == ctrlClient.m_hWnd) {
    ctrlClient.SetWindowText(_T(""));
  }
  return 0;
}

void ChatControl::setMessageText(const TCHAR* text) {
  ctrlMessage.SetWindowText(text);
  ctrlMessage.SetFocus();
  const int len = _tcslen(text);
  ctrlMessage.SetSel(len, len);
}

void ChatControl::addUserToMessageText(const tstring& sUser) {
  int iSelBegin, iSelEnd;
  ctrlMessage.GetSel(iSelBegin, iSelEnd);
  if ((iSelBegin == 0) && (iSelEnd == 0)) {
    CAtlString sText;
    ctrlMessage.GetWindowText(sText);
    tstring text = sUser + _T(": ");
    if (sText.GetLength() == 0) {
      ctrlMessage.SetWindowText(text.c_str());
      ctrlMessage.SetFocus();
      ctrlMessage.SetSel(text.length(), text.length(), FALSE);
    } 
    else {
      ctrlMessage.ReplaceSel(text.c_str());
      ctrlMessage.SetFocus();
    }
  } 
  else {
    tstring text = sUser + _T(": ");
    ctrlMessage.ReplaceSel(text.c_str());
    ctrlMessage.SetFocus();
  }
}

LRESULT ChatControl::onViewLButtonDblClick(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  bHandled = FALSE;
  POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
  int i = ctrlClient.CharFromPos(pt);
  int line = ctrlClient.LineFromChar(i);
  int c = LOWORD(i) - ctrlClient.LineIndex(line);
  int len = ctrlClient.LineLength(i) + 1;
  if (len < 3) {
    return 0;
  }
  AutoArray<TCHAR> buf(len);
  ctrlClient.GetLine(line, buf, len);
  tstring x = tstring(buf, len-1);
  string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
  if (start == string::npos) {
    start = 0;
  }
  else {
    start++;
  }
  string::size_type end = x.find_first_of(_T(" >\t"), start+1);
  if (end == string::npos) { // get EOL as well
    end = x.length();
  }
  else if (end == start + 1) {
    return 0;
  }
  // Nickname click, let's see if we can find one like it in the name list...
  const tstring nick = x.substr(start, end - start);
  UserInfo* ui = m_listener->findUser(nick);
  if (ui) {
    m_listener->onUserDblClick(ui, wParam);
    bHandled = TRUE;
  }
  return 0;
}

LRESULT ChatControl::onViewContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
  if (pt.x == -1 && pt.y == -1) {
    CRect erc;
    ctrlClient.GetRect(&erc);
    pt.x = erc.Width() / 2;
    pt.y = erc.Height() / 2;
    ctrlClient.ClientToScreen(&pt);
  }
  POINT ptCl = pt;
  ctrlClient.ScreenToClient(&ptCl); 
  ctrlClient.OnRButtonDown(ptCl);

  UserInfo *ui = m_listener->findUser(ChatCtrl::sTempSelectedUser);
  bool boHitURL = ctrlClient.HitURL();
  if (!boHitURL) {
    sSelectedURL = Util::emptyStringT;
  }
  m_listener->onUserContextMenu(ui, pt);
  return 0;
}

LRESULT ChatControl::onViewMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
  POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
  RECT rc;
  // Get the bounding rectangle of the client area. 
  ctrlClient.GetClientRect(&rc);
  ctrlClient.ScreenToClient(&pt); 
  if (PtInRect(&rc, pt)) { 
    sSelectedURL = Util::emptyStringT;
  }
  bHandled = FALSE;
  return 0;
} 

void ChatControl::addLine(const Identity& i, const string& nick, const string& extra, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
  ctrlClient.AdjustTextSize();
  if (timeStamps) {
    ctrlClient.AppendText(i, Text::toT(nick), Text::toT("[" + Util::getShortTimeString() + extra + "] "), aLine.c_str(), cf, bUseEmo);
  } 
  else {
    ctrlClient.AppendText(i, Text::toT(nick), !extra.empty() ? Text::toT("[" + extra + "] ") : _T(""), aLine.c_str(), cf, bUseEmo);
  }
}

LRESULT ChatControl::onEmoticonsContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // location of mouse click
  emoMenu.CreateEmotionMenu(pt, m_hWnd, IDC_EMOMENU);
  return TRUE;
}

LRESULT ChatControl::onMessageKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  if (wParam != VK_TAB && !complete.empty()) {
    complete.clear();
  }
  if (wParam == VK_TAB) {
    onTab();
  }
  else if (!processHotKey(wParam)) {
    switch(wParam) 
    {
    case VK_RETURN:
      if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
        bHandled = FALSE;
      } 
      else {
        onSend();
      }
      break;
    case VK_UP:
      if ((GetKeyState(VK_MENU) & 0x8000) || (((GetKeyState(VK_CONTROL) & 0x8000) == 0) ^ (BOOLSETTING(USE_CTRL_FOR_LINE_HISTORY) == true))) {
        //scroll up in chat command history
        //currently beyond the last command?
        if (curCommandPosition > 0) {
          //check whether current command needs to be saved
          if (curCommandPosition == prevCommands.size()) {
            AutoArray<TCHAR> messageContents(ctrlMessage.GetWindowTextLength()+2);
            ctrlMessage.GetWindowText(messageContents, ctrlMessage.GetWindowTextLength()+1);
            currentCommand = tstring(messageContents);
          }
          //replace current chat buffer with current command
          ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
        }
        // move cursor to end of line
        ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
      } 
      else {
        bHandled = FALSE;
      }
      break;
    case VK_DOWN:
      if ((GetKeyState(VK_MENU) & 0x8000) || (((GetKeyState(VK_CONTROL) & 0x8000) == 0) ^ (BOOLSETTING(USE_CTRL_FOR_LINE_HISTORY) == true))) {
        //scroll down in chat command history
        //currently beyond the last command?
        if (curCommandPosition + 1 < prevCommands.size()) {
          //replace current chat buffer with current command
          ctrlMessage.SetWindowText(prevCommands[++curCommandPosition].c_str());
        } 
        else if (curCommandPosition + 1 == prevCommands.size()) {
          //revert to last saved, unfinished command
          ctrlMessage.SetWindowText(currentCommand.c_str());
          ++curCommandPosition;
        }
        // move cursor to end of line
        ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
      } 
      else {
        bHandled = FALSE;
      }
      break;
    case VK_PRIOR: // page up
      ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEUP);
      break;
    case VK_NEXT: // page down
      ctrlClient.SendMessage(WM_VSCROLL, SB_PAGEDOWN);
      break;
    case VK_HOME:
      if (!prevCommands.empty() && (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
        curCommandPosition = 0;
        AutoArray<TCHAR> messageContents(ctrlMessage.GetWindowTextLength()+2);
        ctrlMessage.GetWindowText(messageContents, ctrlMessage.GetWindowTextLength()+1);
        currentCommand = tstring(messageContents);
        ctrlMessage.SetWindowText(prevCommands[curCommandPosition].c_str());
      } 
      else {
        bHandled = FALSE;
      }
      break;
    case VK_END:
      if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
        curCommandPosition = prevCommands.size();
        ctrlMessage.SetWindowText(currentCommand.c_str());
      } 
      else {
        bHandled = FALSE;
      }
      break;
    default:
      bHandled = FALSE;
    }
  }
  return 0;
}

bool ChatControl::processHotKey(WPARAM wParam) {
  if (wParam == VK_ESCAPE) {
    // Clear find text and give the focus back to the message box
    ctrlMessage.SetFocus();
    ctrlClient.SetSel(-1, -1);
    ctrlClient.SendMessage(EM_SCROLL, SB_BOTTOM, 0);
    ctrlClient.InvalidateRect(NULL);
    currentNeedle = Util::emptyStringT;
    return true;
  } 
  else if (wParam == VK_F3 && GetKeyState(VK_SHIFT) & 0x8000) {
    findText(findTextPopup());
    return true;
  } 
  else if (wParam == VK_F3) {
    findText(currentNeedle.empty() ? findTextPopup() : currentNeedle);
    return true;
  }
  return false;
}

LRESULT ChatControl::onMessageChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  if (wParam == VK_TAB) {
    bHandled = TRUE;
  }
  else if (wParam == VK_RETURN) {
    if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
      bHandled = FALSE;
    }
  }
  else {
    bHandled = FALSE;
    if (wParam != VK_BACK) {
	  Sounds::PlaySound(SettingsManager::SOUND_TYPING_NOTIFY);
    }
  }
  return 0;
}

void ChatControl::onTab() {
  if (ctrlMessage.GetWindowTextLength() == 0) {
    //TODO handleTab(WinUtil::isShift());
    return;
  }		
  HWND focus = GetFocus();
  if( (focus == ctrlMessage.m_hWnd) && !WinUtil::isShift() ) 	{
    int n = ctrlMessage.GetWindowTextLength();
    AutoArray<TCHAR> buf(n+1);
    ctrlMessage.GetWindowText(buf, n+1);
    tstring text(buf, n);
    string::size_type textStart = text.find_last_of(_T(" \n\t"));
    if (complete.empty()) {
      if (textStart != string::npos) {
        complete = text.substr(textStart + 1);
      } 
      else {
        complete = text;
      }
      if (complete.empty()) {
        // Still empty, no text entered...
        m_listener->autoCompleteFaliure();
        return;
      }
      m_listener->autoCompleteBegin();
    }
    const tstring completedNick = m_listener->autoCompleteUserNick(complete);
    if (!completedNick.empty()) {
      if (textStart == string::npos) {
        textStart = 0;
      }
      else {
        textStart++;
      }
      ctrlMessage.SetSel(textStart, ctrlMessage.GetWindowTextLength(), TRUE);
      ctrlMessage.ReplaceSel(completedNick.c_str());
    }
  }
}

LRESULT ChatControl::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if ((GetFocus() == ctrlClient.m_hWnd) && (ChatCtrl::sSelectedLine != _T(""))) {
    WinUtil::setClipboard(ChatCtrl::sSelectedLine);
  }
  return 0;
}

LRESULT ChatControl::onAutoScrollChat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  ctrlClient.SetAutoScroll( !ctrlClient.GetAutoScroll() );
  return 0;
}

LRESULT ChatControl::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (!sSelectedURL.empty()) {
    WinUtil::setClipboard(sSelectedURL);
  }
  return 0;
}

void ChatControl::updateColors() {
  ctrlClient.SetBackgroundColor(WinUtil::bgColor);
}

LRESULT ChatControl::onSizeMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  bHandled = FALSE;
  ctrlClient.GoToEnd();
  return 0;
}

LRESULT ChatControl::onViewKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  if (!processHotKey(wParam)) {
    bHandled = FALSE;
  }
  return 0;
}
