#include "../../client/stdinc.h"
#include "ZipInputStream.h"

bool ZipInputStream::open(const char* filename) {
  if (m_zipFile != NULL) {
    close();
  }
  zlib_filefunc_def ffunc;
  fill_win32_filefunc(&ffunc);
  m_zipFile = unzOpen2(filename, &ffunc);
  return m_zipFile != NULL;
}

void ZipInputStream::processEntries() {
  unz_global_info gi;
  int code = unzGetGlobalInfo(m_zipFile, &gi);
  if (code != UNZ_OK) {
    throw ZipException("Error unzGetGlobalInfo", code);
  }
  for (uLong i = 0; i < gi.number_entry; ++i) {
    char filename_inzip[MAX_PATH];
    unz_file_info file_info;
    code = unzGetCurrentFileInfo(m_zipFile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
    if (code != UNZ_OK) {
      throw ZipException("Error unzGetCurrentFileInfo", code);
    }
#ifdef _DEBUG
    printf("file \"%s\" version=%d internal_fa=%d external_fa=%d\n", filename_inzip, file_info.version, file_info.internal_fa, file_info.external_fa);
#endif
    code = unzOpenCurrentFile(m_zipFile);
    if (code != UNZ_OK) {
      throw ZipException("error unzOpenCurrentFile", code);
    }
    processEntry(filename_inzip, file_info);
    if (i + 1 < gi.number_entry) {
      code = unzGoToNextFile(m_zipFile);
      if (code != UNZ_OK) {
        throw ZipException("Error unzGoToNextFile", code);
      }
    }
  }
}

void ZipInputStream::skipEntry() {
  char buffer[4096];
  while (readCurrentFile(buffer, sizeof(buffer)) != 0) {
  }
}

void ZipInputStream::processEntry(const char* entryName, const unz_file_info& fileInfo) {
  skipEntry();
}

size_t ZipInputStream::readCurrentFile(void* buffer, size_t bufferSize) {
  int count = unzReadCurrentFile(m_zipFile, buffer, (int) bufferSize);
  if (count < 0) {
    throw ZipException("error in unzReadCurrentFile", count);
  }
  else if (count == 0) {
    count = unzCloseCurrentFile(m_zipFile);
    if (count < 0) {
      throw ZipException("error in unzCloseCurrentFile", count);
    }
    return 0;
  }
  else {
    return count;
  }
}
