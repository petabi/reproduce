#ifndef OPTIONS_H
#define OPTIONS_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#define F __func__

enum class InputType {
  NONE,   // no type
  PCAP,   // pcap file
  PCAPNG, // pcapng file
  NIC,    // network interface
  LOG,    // log file
};

struct Config {
  bool mode_debug{false}; // debug mode (print debug messages)
  bool mode_eval{false};  // evaluation mode (report statistics)
  bool mode_kafka{false}; // do not send data to kafka (parse packet only)
  bool mode_parse{false}; // do not parse packet (send hardcoded sample data)
  size_t count_send{0};   // send packet count
  size_t count_skip{0};   // skip packet count
  size_t queue_size{0};   // queue size (how many bytes send once)
  std::string input;      // input pcapfile or nic
  std::string output;     // output file
  std::string filter;     // tcpdump filter string
  std::string broker;     // kafka broker
  std::string topic;      // kafka topic
};

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
  void fprint(const char* message) noexcept;
  void show_options() const noexcept;
  bool check_count() const noexcept;
  void start_evaluation() noexcept;
  void process_evaluation(int length) noexcept;
  void report_evaluation() const noexcept;
  bool open_output_file() noexcept;
  void increase_fail() noexcept;
  InputType get_input_type() noexcept;

private:
  void set_input_type() noexcept;
  size_t sent_byte;          // sent bytes
  size_t sent_packet;        // sent packets
  size_t fail_packet;        // failed packet count
  double perf_kbps;          // kilo byte per second
  double perf_kpps;          // kilo packet per second
  clock_t time_start;        // start time
  clock_t time_now;          // current time
  double time_diff;          // time difference
  InputType input_type;      // input type
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
