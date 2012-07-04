#define PEERS_UPDATE_ZIP "PeersUpdate.zip"
#define PEERS_UPDATER_EXE "PeersUpdater.exe"
#define PEERS_EXE "Peers.exe"
/* �������� ��������� ������ ��� ������� ���������� ���������� */
#define PEERS_OPTION_NO_UPDATE "/noupdate"

class AutoUpdate {
private:
  /* ���������� ���� � �������� ���������� ������� ��������� ���� */
  tstring static getApplicationDirectory();
public:
  enum {
    /* ��� ������, ����� ���������� ����������� ��� ����-���������� */
    EXIT_TO_UPDATE = 10
  };

  /* ���������� ����, � ������� ����� ��������� ���������� */
  tstring static getUpdateTargetPath();

  /* ���������� ��������������. ���������� true, ���� ������� ������� ��������� */
  bool static execute();

  /* ��������� ��������� ���� - ��� �� ����������, � ���� ��������� ������ � md5 */
  bool static checkFile(const tstring& fileName, long fileSize, const string& md5);

#ifdef _DEBUG
  /* ����� �� ��������� �������� ���������� ��� ������ */
  bool static allowOnStart();

  /* ����� �� ��������� ������������� �������� ���������� */
  bool static allowPeriodic();

  /* ���������� �������� ���������� ������������� �������� ����������, � �������� */
  int static getUpdateCheckInterval();
#else
  /* ����� �� ��������� �������� ���������� ��� ������ */
  bool static inline allowOnStart() { return true; }

  /* ����� �� ��������� ������������� �������� ���������� */
  bool static inline allowPeriodic() { return true; }

  /* ���������� �������� ���������� ������������� �������� ����������, � �������� */
  int static inline getUpdateCheckInterval() { return 60 * 60; }
#endif
};
