#pragma once
#include "FlatIconButton.h"

class FlatMultiIconButton : public FlatIconButton {
private:
  int m_iconIndex;
  CIcon m_altIcon;
protected:
  virtual CIcon& getCurrentIcon() { 
    return m_iconIndex == 0 ? FlatIconButton::getCurrentIcon() : m_altIcon;
  }
public:
  FlatMultiIconButton(): m_iconIndex(0) { }

  void loadAltIcon(ATL::_U_STRINGorID resourceId) {
    m_altIcon.LoadIcon(resourceId);
  }

  int getIconIndex() const {
    return m_iconIndex;
  }

  bool setIconIndex(int value) {
    if (value != m_iconIndex) {
      m_iconIndex = value;
      Invalidate();
      return true;
    }
    else {
      return false;
    }
  }
};
