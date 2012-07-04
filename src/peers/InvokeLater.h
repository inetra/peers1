#pragma once

class InvokeLater : public CWindowImpl<InvokeLater, CWindow, CNullTraits> {
public:
	class Listener {
	public:
		virtual bool executeLater() = 0;
	};
	InvokeLater(Listener* listener, int delay = 1000): m_listener(listener), m_delay(delay) {
		Create(NULL);
	}
	void execute() {
		SetTimer(TIMER_ID, m_delay);
	}
	virtual void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}
protected:
	virtual bool executeAction() {
		return m_listener->executeLater();
	}
	virtual ~InvokeLater() {
	}
private:
	enum { TIMER_ID = 1 };
	const int m_delay;
	Listener* m_listener;
	BEGIN_MSG_MAP(PopupManagerHiddenWindow)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
	END_MSG_MAP()
	LRESULT onTimer(UINT, WPARAM, LPARAM, BOOL&) { if (executeAction()) { KillTimer(TIMER_ID); DestroyWindow(); } return 0; }
};
