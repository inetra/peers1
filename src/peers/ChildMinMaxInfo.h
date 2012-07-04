#pragma once

class ChildMinMaxInfo {
public:
  ChildMinMaxInfo(): m_minHeight(0) { }
  int m_minHeight;
};

typedef ChildMinMaxInfo* PChildMinMaxInfo;
