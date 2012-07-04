#include "stdinc.h"
#include "DCPlusPlus.h"
#include "ChunkLog.h"

#include "Text.h"
#include "File.h"
#include "User.h"


static CriticalSection& getCriticalSection() {
	static CriticalSection cs;
	return cs;
}

static File* lastFile = NULL;
static TTHValue lastTTH;

ChunkLog::~ChunkLog() {
	Lock lock(getCriticalSection());
	if (lastFile != NULL && m_tth == lastTTH) {		
		delete lastFile;
		lastFile = NULL;
	}
}

void CDECL ChunkLog::logDownload(const char* format, ...) {
	va_list args;
	va_start(args, format);
	char message[1024];
	_vsnprintf(message, sizeof(message), format, args);	
	Lock lock(getCriticalSection());
	SYSTEMTIME st;
	GetLocalTime(&st);
	char timestamp[256];
	snprintf(timestamp, sizeof(timestamp), 
		"%04d-%02d-%02d %02d:%02d:%02d.%03d [%08x] %-10s: %5s%-20s ", 
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, 
		GetCurrentThreadId(),
		Text::utf8ToAcp(Util::getFileName(m_target)).c_str(),
		m_user ? "user=" : "",
		m_user ? Text::utf8ToAcp(m_user->getFirstNick()).c_str() : ""
	);
	File *f;
	if (lastFile != NULL && m_tth == lastTTH) {
		f = lastFile;
	}
	else {
		string path = SETTING(LOG_DIRECTORY) + "downloads\\" + m_tth.toBase32() + ".txt";
		File::ensureDirectory(path);
		f = new File(path, File::WRITE, File::OPEN | File::CREATE);
		f->setEndPos(0);
		if (lastFile != NULL) {
			delete lastFile;
		}
		lastFile = f;
		lastTTH = m_tth;
	}
	f->write(string(timestamp) + Text::utf8ToAcp(message) + "\r\n");
}
