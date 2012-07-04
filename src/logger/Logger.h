#ifndef _LOGGER_H_
#define _LOGGER_H_

#pragma once

#include <string>
using namespace std;

#define __LOGGER_CDECL _cdecl
#define ASSIGNMENT_OPERATOR_UNAVAILABLE(TYPE) void operator = (const TYPE&);
#define LOGGER_NS_BEGIN namespace Logger {
#define LOGGER_NS_END   };

LOGGER_NS_BEGIN

class Level {
  friend class Repository;
private:
  int m_id;
  const char* m_name;

  Level(int id, const char* name): m_id(id), m_name(name) {
  }
public:
  const char* c_str() const { return m_name; }
};

class CategoryNode {
  friend class Repository;
  friend class Category;
  ASSIGNMENT_OPERATOR_UNAVAILABLE(CategoryNode);
private:
  const string m_name;
protected:
  explicit CategoryNode(const string& name): m_name(name) { }
  virtual ~CategoryNode() { }
};

class Category {
  friend class Repository;
private:
  const CategoryNode* m_categoryNode;
public:
  explicit Category(const char* categoryName);
};

class Appender {
  friend class Repository;
protected:
  virtual void write(const Level& level, const char* format, va_list args) = 0;
public:
  virtual void printf(const Level& level, const char* format, ...);
  virtual ~Appender() { }
};

class NullAppender : public Appender {
protected:
  virtual void write(const Level& /*level*/, const char* /*format*/, va_list /*args*/) { }
};

class Repository {
  friend class Category;
private:
  static CategoryNode* registerCategory(const char* categoryName);
public:
  static void registerAppender(const Category& category, Appender* appender);
  static bool hasAppender(const Category& category);

  static void __LOGGER_CDECL log(const Category& category, const Level& level, const char* format, va_list args);

  static Level DEBUG_LEVEL;
  static Level INFO_LEVEL;
  static Level WARN_LEVEL;
  static Level ERROR_LEVEL;

  static Category DEFAULT_CATEGORY;

};

inline Category::Category(const char* categoryName): m_categoryNode(Repository::registerCategory(categoryName)) {
}

void __LOGGER_CDECL debug(const char* format, ...);
void __LOGGER_CDECL info(const char* format, ...);
void __LOGGER_CDECL warn(const char* format, ...);
void __LOGGER_CDECL error(const char* format, ...);

void __LOGGER_CDECL debug(const Category& category, const char* format, ...);
void __LOGGER_CDECL info(const Category& category, const char* format, ...);
void __LOGGER_CDECL warn(const Category& category, const char* format, ...);
void __LOGGER_CDECL error(const Category& category, const char* format, ...);

LOGGER_NS_END

#define LOGGER_CATEGORY(name) Logger::Category name(#name);
#define EXTERN_LOGGER_CATEGORY(name) extern Logger::Category name;
#define LOGGER_CATEGORY_DECLARE_STATIC_MEMBER(name) static Logger::Category name;
#define LOGGER_CATEGORY_IMPLEMENT_STATIC_MEMBER(classname,name) Logger::Category classname::name(#name);

#ifdef _DEBUG
#define debug_log  Logger::debug
#define info_log   Logger::info
#define warn_log   Logger::warn
#define error_log  Logger::error
#else
#define debug_log  Logger::debug
#define info_log   Logger::info
#define warn_log   Logger::warn
#define error_log  Logger::error
#endif

#endif /* _LOGGER_H_ */
// $Id: Logger.h,v 1.1.2.1 2008/06/06 17:25:54 alexey Exp $
