#include "stdafx.h"
#include "ChatCommand.h"

ChatCommandContext::ChatCommandContext(const tstring& s) {
  dcassert(isCommand(s));
  const string::size_type i = s.find(' ');
  if (i != string::npos) {
    param = s.substr(i + 1);
    command = s.substr(1, i - 1);
  } 
  else {
    command = s.substr(1);
  }
}
