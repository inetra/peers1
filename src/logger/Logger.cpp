#include "Logger.h"
#include <vector>
#include <stdarg.h>

#if defined(_MSC_VER) && defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)
#define new DEBUG_NEW
static char THIS_FILE[] = __FILE__;
#endif

LOGGER_NS_BEGIN

Category Repository::DEFAULT_CATEGORY("DEFAULT");

Level Repository::DEBUG_LEVEL(0, "DEBUG");
Level Repository::INFO_LEVEL(1, "INFO");
Level Repository::WARN_LEVEL(2, "WARN");
Level Repository::ERROR_LEVEL(3, "ERROR");

void Appender::printf(const Level& level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  write(level, format, args);
  va_end(args);
}

class CategoryNodeData : public CategoryNode {
  friend class Repository;
private:
  Appender* m_appender;
  explicit CategoryNodeData(const string& name): CategoryNode(name), m_appender(NULL) {
  }
public:
  virtual ~CategoryNodeData() {
    if (m_appender != NULL) {
      delete m_appender;
    }
  }
};

CategoryNode* Repository::registerCategory(const char *categoryName) {
	static vector<CategoryNodeData*> categories;
	string name(categoryName);
	for (vector<CategoryNodeData*>::iterator i = categories.begin(); i != categories.end(); ++i) {
		CategoryNodeData* c = *i;
		if (c->m_name == name) {
			return c;
		}
	}
	CategoryNodeData* newNode = new CategoryNodeData(name);
	categories.push_back(newNode);
	return newNode;
}

void Repository::registerAppender(const Logger::Category &category, Logger::Appender *appender) {
  ((CategoryNodeData*) category.m_categoryNode)->m_appender = appender;
}

bool Repository::hasAppender(const Logger::Category& category) {
  return ((CategoryNodeData*) category.m_categoryNode)->m_appender != NULL;
}

void Repository::log(const Category& category, const Level& level, const char* format, va_list args) {
  Appender* appender = static_cast<const CategoryNodeData*>(category.m_categoryNode)->m_appender;
  if (appender != NULL) {
    appender->write(level, format, args);
  }
}

void __LOGGER_CDECL debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(Repository::DEFAULT_CATEGORY, Repository::DEBUG_LEVEL, format, args);
  va_end(args);
}

void __LOGGER_CDECL info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(Repository::DEFAULT_CATEGORY, Repository::INFO_LEVEL, format, args);
  va_end(args);
}

void __LOGGER_CDECL warn(const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(Repository::DEFAULT_CATEGORY, Repository::WARN_LEVEL, format, args);
  va_end(args);
}

void __LOGGER_CDECL error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(Repository::DEFAULT_CATEGORY, Repository::ERROR_LEVEL, format, args);
  va_end(args);
}

void debug(const Category& category, const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(category, Repository::DEBUG_LEVEL, format, args);  
  va_end(args);
}

void info(const Category& category, const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(category, Repository::INFO_LEVEL, format, args);  
  va_end(args);
}

void warn(const Category& category, const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(category, Repository::WARN_LEVEL, format, args);  
  va_end(args);
}

void error(const Category& category, const char* format, ...) {
  va_list args;
  va_start(args, format);
  Repository::log(category, Repository::ERROR_LEVEL, format, args);  
  va_end(args);
}

LOGGER_NS_END
// $Id: Logger.cpp,v 1.1.2.1 2008/06/06 17:25:54 alexey Exp $
