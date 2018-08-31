#ifndef UTIL_H
#define UTIL_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#define F __func__

class Util {
public:
  Util() = default;
  Util(const bool);
  Util(const Util&) = default;
  Util& operator=(const Util&) = default;
  Util(Util&&) = delete;
  Util& operator=(Util&&) = delete;
  ~Util() = default;
  template <typename T> void print(T tail) const;
  template <typename T, typename... Ts> void print(T head, Ts... tail) const;
  template <typename T> void dprint(const char* name, T tail) const;
  template <typename T, typename... Ts>
  void dprint(const char* name, T head, Ts... tail) const;
  template <typename T> void eprint(const char* name, T tail) const;
  template <typename T, typename... Ts>
  void eprint(const char* name, T head, Ts... tail) const;
  void mprint(const char* message) const noexcept;
  void set_debug(const bool& debug);

private:
  bool debug{false};
};

template <typename T> void Util::print(T tail) const
{
  std::cout << tail << "\n";
}

template <typename T, typename... Ts> void Util::print(T head, Ts... tail) const
{
  std::cout << head;
  print(tail...);
}

template <typename T> void Util::dprint(const char* name, T head) const
{
  if (!debug) {
    return;
  }

  std::cout << "[DEBUG] " << name << ": " << head << "\n";
}

template <typename T, typename... Ts>
void Util::dprint(const char* name, T head, Ts... tail) const
{
  if (!debug) {
    return;
  }

  std::cout << "[DEBUG] " << name << ": " << head;
  print(tail...);
}

template <typename T> void Util::eprint(const char* name, T tail) const
{
  std::cout << "[ERROR] " << name << ": " << tail << "\n";
}

template <typename T, typename... Ts>
void Util::eprint(const char* name, T head, Ts... tail) const
{
  std::cout << "[ERROR] " << name << ": " << head;
  print(tail...);
}

#endif

// vim: et:ts=2:sw=2
