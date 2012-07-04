#pragma once

template<typename T> class StaticAccessSingleton {
public:
  StaticAccessSingleton() { dcassert(instance==NULL); instance = static_cast<T*>(this); }

  virtual ~StaticAccessSingleton() { dcassert(instance == this); instance = NULL; }

  static T* getInstance() {
    return instance;
  }

private:
  static T* instance;
};

template<class T> T* StaticAccessSingleton<T>::instance = NULL;
