#ifndef UTIL_H
#define UTIL_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#define F __func__

class Util {
public:
  Util() = delete;
  Util(const Util&) = delete;
  Util& operator=(const Util&) = delete;
  Util(Util&&) = delete;
  Util& operator=(Util&&) = delete;
  ~Util() = delete;
  template <typename T> static void print(T tail);
  template <typename T, typename... Ts> static void print(T head, Ts... tail);
  template <typename T> static void dprint(const char* name, T tail);
  template <typename T, typename... Ts>
  static void dprint(const char* name, T head, Ts... tail);
  template <typename T> static void eprint(const char* name, T tail);
  template <typename T, typename... Ts>
  static void eprint(const char* name, T head, Ts... tail);
  static void mprint(const char* message, const size_t count) noexcept;
  static void set_debug(const bool& debug);
  static std::string del_space(std::string& str);

private:
  static bool debug;
};

template <typename T> void Util::print(T tail) { std::cout << tail << "\n"; }

template <typename T, typename... Ts> void Util::print(T head, Ts... tail)
{
  std::cout << head;
  print(tail...);
}

template <typename T> void Util::dprint(const char* name, T head)
{
  if (!debug) {
    return;
  }

  std::cout << "[DEBUG] " << name << ": " << head << "\n";
}

template <typename T, typename... Ts>
void Util::dprint(const char* name, T head, Ts... tail)
{
  if (!debug) {
    return;
  }

  std::cout << "[DEBUG] " << name << ": " << head;
  print(tail...);
}

template <typename T> void Util::eprint(const char* name, T tail)
{
  std::cout << "[ERROR] " << name << ": " << tail << "\n";
}

template <typename T, typename... Ts>
void Util::eprint(const char* name, T head, Ts... tail)
{
  std::cout << "[ERROR] " << name << ": " << head;
  print(tail...);
}

#endif

// vim: et:ts=2:sw=2
