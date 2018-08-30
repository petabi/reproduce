#ifndef OPTIONS_H
#define OPTIONS_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "config.h"

#define F __func__

class Options {
public:
  Config conf;
  Options() = default;
  Options(Config);
  Options(const Options&);
  Options& operator=(const Options&);
  Options(Options&&) = delete;
  Options& operator=(Options&&) = delete;
  ~Options();
  template <typename T> void print(T tail) const;
  template <typename T, typename... Ts> void print(T head, Ts... tail) const;
  template <typename T> void dprint(const char* name, T tail) const;
  template <typename T, typename... Ts>
  void dprint(const char* name, T head, Ts... tail) const;
  template <typename T> void eprint(const char* name, T tail) const;
  template <typename T, typename... Ts>
  void eprint(const char* name, T head, Ts... tail) const;
  void mprint(const char* message) const noexcept;
  void show_options() const noexcept;
  bool check_count() const noexcept;
  void start_evaluation() noexcept;
  void process_evaluation(int length) noexcept;
  void report_evaluation() const noexcept;
  bool open_output_file() noexcept;
  void increase_fail() noexcept;

private:
  size_t sent_byte{0};       // sent bytes
  size_t sent_packet{0};     // sent packets
  size_t fail_packet{0};     // failed packet count
  double perf_kbps{0.0};     // kilo byte per second
  double perf_kpps{0.0};     // kilo packet per second
  clock_t time_start{0};     // start time
  clock_t time_now{0};       // current time
  double time_diff{0.0};     // time difference
  std::ofstream output_file; // output file
};

template <typename T> void Options::print(T tail) const
{
  std::cout << tail << "\n";
}

template <typename T, typename... Ts>
void Options::print(T head, Ts... tail) const
{
  std::cout << head;
  print(tail...);
}

template <typename T> void Options::dprint(const char* name, T head) const
{
  if (!conf.mode_debug) {
    return;
  }

  std::cout << "[DEBUG] " << name << ": " << head << "\n";
}

template <typename T, typename... Ts>
void Options::dprint(const char* name, T head, Ts... tail) const
{
  if (!conf.mode_debug) {
    return;
  }

  std::cout << "[DEBUG] " << name << ": " << head;
  print(tail...);
}

template <typename T> void Options::eprint(const char* name, T tail) const
{
  std::cout << "[ERROR] " << name << ": " << tail << "\n";
}

template <typename T, typename... Ts>
void Options::eprint(const char* name, T head, Ts... tail) const
{
  std::cout << "[ERROR] " << name << ": " << head;
  print(tail...);
}

#endif

// vim: et:ts=2:sw=2
