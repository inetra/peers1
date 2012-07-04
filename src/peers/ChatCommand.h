#pragma once

/* контекст обработки команд в окне чата */
class ChatCommandContext {
public:
  /* проверка что строка является командой */
  static bool isCommand(const tstring& s) { return !s.empty() && s[0] == '/'; }
  /* конструктор */
  ChatCommandContext(const tstring& s);
  /* нужно ли очищать окно ввода сообщений TODO возможно какие-то другие параметры на эту тему ?*/
  bool clearMessageWindow;
  /* команда */
  tstring command;
  /* параметр(ы) */
  tstring param;

  tstring getCommandLine() const { return _T("/") + command + _T(" ")+ param; }
};

