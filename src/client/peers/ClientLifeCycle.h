#pragma once

class StartupConfiguration {
public:
  string m_clientId;
  string m_queueManagerVersion;
  string m_settingsManagerVersion;
  string m_mainHub;
  virtual ~StartupConfiguration() { }
};

class ProgressCallback {
public:
  virtual void showMessage(const tstring& message) = 0;
};

extern void startup(ProgressCallback* callback, const StartupConfiguration* configuration);
extern void shutdown(bool exp = false);

/**
 * $Id: ClientLifeCycle.h,v 1.2 2008/03/13 17:45:26 alexey Exp $
 */
