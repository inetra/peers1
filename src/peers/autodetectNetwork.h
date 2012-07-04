#pragma once
#include "../client/HttpConnection.h"
#include "InvokeLater.h"

class DetectNetworkSetupThread : public PointerBase, HttpConnectionListener, InvokeLater::Listener {
private:
	class DetectHttpConnection : public HttpConnection {
	public:
		string m_localIp;
		DetectHttpConnection(const string& userAgent): HttpConnection(userAgent) { }
		void on(BufferedSocketListener::Connected) throw() { 
			dcassert(socket); 
			m_localIp = socket->getLocalIp();
			HttpConnection::on(BufferedSocketListener::Connected());
		}
	};

	InvokeLater* m_invoker;
	volatile bool m_done;
	DetectHttpConnection connection;
	string m_buffer;
	virtual void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t *buf , size_t len) {
		m_buffer += string((const char*)buf, len);
	}
	virtual void on(Complete, HttpConnection*, const string&) throw();
	virtual void on(Failed, HttpConnection*, const string& message) throw();
	void finalize() {
		m_done = true;
		m_invoker->execute();
	}
	virtual bool executeLater() {
		dec();
		return true;
	}
public:
	DetectNetworkSetupThread();

#ifdef _DEBUG
	~DetectNetworkSetupThread() {
		dcdebug("~DetectNetworkSetupThread\n");
	}
#endif

	bool isDone() const {
		return m_done;
	}

	void wait();

	static void run();
};
