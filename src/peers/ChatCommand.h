#pragma once

/* �������� ��������� ������ � ���� ���� */
class ChatCommandContext {
public:
  /* �������� ��� ������ �������� �������� */
  static bool isCommand(const tstring& s) { return !s.empty() && s[0] == '/'; }
  /* ����������� */
  ChatCommandContext(const tstring& s);
  /* ����� �� ������� ���� ����� ��������� TODO �������� �����-�� ������ ��������� �� ��� ���� ?*/
  bool clearMessageWindow;
  /* ������� */
  tstring command;
  /* ��������(�) */
  tstring param;

  tstring getCommandLine() const { return _T("/") + command + _T(" ")+ param; }
};

