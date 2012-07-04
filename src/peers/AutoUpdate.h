#define PEERS_UPDATE_ZIP "PeersUpdate.zip"
#define PEERS_UPDATER_EXE "PeersUpdater.exe"
#define PEERS_EXE "Peers.exe"
/* параметр командной строки для запрета выполнения обновления */
#define PEERS_OPTION_NO_UPDATE "/noupdate"

class AutoUpdate {
private:
  /* возвращает путь к каталогу приложения включая последний слеш */
  tstring static getApplicationDirectory();
public:
  enum {
    /* код выхода, когда приложение завершается для авто-обновления */
    EXIT_TO_UPDATE = 10
  };

  /* возвращает путь, в который нужно скачивать обновления */
  tstring static getUpdateTargetPath();

  /* выполнение автообновления. возвращает true, если процесс удалось запустить */
  bool static execute();

  /* проверяет указанный файл - что он существует, у него указанные размер и md5 */
  bool static checkFile(const tstring& fileName, long fileSize, const string& md5);

#ifdef _DEBUG
  /* нужно ли выполнять проверку обновлений при старте */
  bool static allowOnStart();

  /* нужно ли выполнять периодическую проверку обновлений */
  bool static allowPeriodic();

  /* возвращает интервал выполнения периодической проверки обновлений, в секундах */
  int static getUpdateCheckInterval();
#else
  /* нужно ли выполнять проверку обновлений при старте */
  bool static inline allowOnStart() { return true; }

  /* нужно ли выполнять периодическую проверку обновлений */
  bool static inline allowPeriodic() { return true; }

  /* возвращает интервал выполнения периодической проверки обновлений, в секундах */
  int static inline getUpdateCheckInterval() { return 60 * 60; }
#endif
};
