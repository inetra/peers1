#include "stdafx.h"
#include "Application.h"
#include "PopupManager.h"
#include "ToolbarManager.h"

void Application::startup(ProgressCallback* callback, const StartupConfiguration* configuration) {
  ::startup(callback, configuration);
  PopupManager::newInstance();
  ToolbarManager::newInstance();
}

void Application::shutdown(bool exp /*= false*/) {
  ToolbarManager::deleteInstance();
  PopupManager::deleteInstance();
  ::shutdown(exp);
}
