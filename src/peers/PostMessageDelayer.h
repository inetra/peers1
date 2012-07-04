#pragma once

class PostMessageDelayer : public CWindowImpl<PostMessageDelayer, CWindow, CNullTraits> {
  BEGIN_MSG_MAP(PostMessageDelayer)
    MESSAGE_HANDLER(WM_TIMER, onTimer)
  END_MSG_MAP()
  LRESULT onTimer(UINT, WPARAM, LPARAM, BOOL&) {
    ::PostMessage(m_targetWnd, m_msg, m_wParam, m_lParam);
    DestroyWindow();
    return 0;
  }
  UINT m_msg;
  WPARAM m_wParam;
  LPARAM m_lParam;
  HWND m_targetWnd;
  PostMessageDelayer(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam):m_targetWnd(wnd), m_msg(msg), m_wParam(wParam), m_lParam(lParam) { }
  virtual ~PostMessageDelayer() {}
  virtual void OnFinalMessage(HWND ) {
    delete this;
  }
public:
  static void execute(int delay, HWND wnd, UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) {
    PostMessageDelayer* delayer = new PostMessageDelayer(wnd, msg, wparam, lparam);
    delayer->Create(wnd);
    delayer->SetTimer(1, delay);
  }
};
