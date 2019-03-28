#ifndef UTIL_H
#define UTIL_H

#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <syslog.h>

inline std::string class_name(const std::string& pretty_function)
{
  size_t colons = pretty_function.find("::");
  if (colons == std::string::npos)
    return "";
  size_t begin = pretty_function.substr(0, colons).rfind(' ') + 1;
  size_t end = colons - begin;

  return pretty_function.substr(begin, end);
}

#define __CLASS__ class_name(__PRETTY_FUNCTION__)
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define F (__CLASS__).c_str(), __func__

class Util {
public:
  Util() = delete;
  Util(const Util&) = delete;
  Util& operator=(const Util&) = delete;
  Util(Util&&) = delete;
  Util& operator=(Util&&) = delete;
  ~Util() = delete;
  template <typename T> static void print(std::ostream& logstream, T tail);
  template <typename T, typename... Ts>
  static void print(std::ostream& logstream, T head, Ts... tail);
  template <typename T>
  static void dprint(const char* file, const char* name, T tail);
  template <typename T, typename... Ts>
  static void dprint(const char* file, const char* name, T head, Ts... tail);
  template <typename T> static void iprint(T tail);
  template <typename T, typename... Ts> static void iprint(T head, Ts... tail);
  template <typename T> static void eprint(T tail);
  template <typename T, typename... Ts> static void eprint(T head, Ts... tail);
  static std::string& ltrim(std::string& str);
  static std::string& rtrim(std::string& str);
  static std::string& trim(std::string& str);

private:
};

template <typename T> void Util::print(std::ostream& logstream, T tail)
{
  logstream << tail << "\n";
}

template <typename T, typename... Ts>
void Util::print(std::ostream& logstream, T head, Ts... tail)
{
  logstream << head;
  print(logstream, tail...);
}

template <typename T>
void Util::dprint(const char* file, const char* name, T head)
{
#ifdef DEBUG
  std::ostringstream logstream;
  logstream << file << "::" << name << ": " << head << "\n";
  std::cout << logstream.str();
  syslog(LOG_DEBUG, "%s", (logstream.str()).c_str());
#endif
}

template <typename T, typename... Ts>
void Util::dprint(const char* file, const char* name, T head, Ts... tail)
{
#ifdef DEBUG
  std::ostringstream logstream;
  logstream << file << "::" << name << ": " << head;
  print(logstream, tail...);
  std::cout << logstream.str();
  syslog(LOG_DEBUG, "%s", (logstream.str()).c_str());
#endif
}

template <typename T> void Util::iprint(T tail)
{
  std::ostringstream logstream;
  logstream << tail << "\n";
  std::cout << logstream.str();
#ifdef DEBUG
  syslog(LOG_INFO, "%s", (logstream.str()).c_str());
#endif
}

template <typename T, typename... Ts> void Util::iprint(T head, Ts... tail)
{
  std::ostringstream logstream;
  logstream << head;
  print(logstream, tail...);
  std::cout << logstream.str();
#ifdef DEBUG
  syslog(LOG_INFO, "%s", (logstream.str()).c_str());
#endif
}

template <typename T> void Util::eprint(T tail)
{
  std::ostringstream logstream;
  logstream << tail << "\n";
  std::cout << logstream.str();
#ifdef DEBUG
  syslog(LOG_ERR, "%s", (logstream.str()).c_str());
#endif
}

template <typename T, typename... Ts> void Util::eprint(T head, Ts... tail)
{
  std::ostringstream logstream;
  logstream << head;
  print(logstream, tail...);
  std::cout << logstream.str();
#ifdef DEBUG
  syslog(LOG_ERR, "%s", (logstream.str()).c_str());
#endif
}

#endif

// vim: et:ts=2:sw=2
