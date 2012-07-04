#pragma once
#include "../client/peers/ClientLifeCycle.h"

class Application {
public:
  static void startup(ProgressCallback* callback, const StartupConfiguration* configuration);
  static void shutdown(bool exp = false);
};
