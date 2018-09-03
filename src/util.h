#ifndef UTIL_H
#define UTIL_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#define F __func__

class Util {
public:
  /*
    Util() = default;
    Util(const bool);
    Util(const Util&) = default;
    Util& operator=(const Util&) = default;
    Util(Util&&) = delete;
    Util& operator=(Util&&) = delete;
    ~Util() = default;
  */
  template <typename T> static void print(T tail);
  template <typename T, typename... Ts> static void print(T head, Ts... tail);
  template <typename T> static void dprint(const char* name, T tail);
  template <typename T, typename... Ts>
  static void dprint(const char* name, T head, Ts... tail);
  template <typename T> static void eprint(const char* name, T tail);
  template <typename T, typename... Ts>
  static void eprint(const char* name, T head, Ts... tail);
  static void mprint(const char* message) noexcept;
  static void set_debug(const bool& debug);

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
