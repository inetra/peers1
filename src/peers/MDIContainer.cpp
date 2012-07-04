#include "stdafx.h"

static map<HWND,MDIContainer::Window> children;

void MDIContainer::onCreate(HWND hwnd, MDIContainer::Window child) {
  children[hwnd] = child;
}

void MDIContainer::onDestroy(HWND hwnd) {
  children.erase(hwnd);
}

MDIContainer::Window MDIContainer::get(HWND hwnd) {
  map<HWND,MDIContainer::Window>::iterator i = children.find(hwnd);
  return i != children.end() ? (*i).second : NULL;
}
 
MDIContainer::Window MDIContainer::getActive() {
  return get(WinUtil::MDIGetActive());
}

vector<MDIContainer::Window> MDIContainer::list() {
  vector<MDIContainer::Window> result;
  for (map<HWND,MDIContainer::Window>::iterator i = children.begin(); i != children.end(); ++i) {
    result.push_back((*i).second);
  }
  return result;
}
