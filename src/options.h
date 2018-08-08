#ifndef OPTIONS_H
#define OPTIONS_H

#include <ctime>
#include <string>

#define F __func__

enum class InputType {
  None, // no type
  File, // file name
  Nic,  // network interface name
};

class Options {
public:
  bool debug;         // debug mode (print debug messages)
  bool eval;          // evaluation mode (report statistics)
  bool kafka;         // do not send to kafka
  size_t count;       // packet count to send
  size_t skip;        // skip packet count
  std::string input;  // input pcapfile or nic
  std::string output; // output file
  std::string filter; // tcpdump filter string
  std::string broker; // kafka broker
  std::string topic;  // kafka topic

  Options();
  Options(const Options&) = delete;
  Options& operator=(const Options&) = delete;
  Options(Options&&) = delete;
  Options& operator=(Options&&) = delete;
  ~Options();
  void show_options() noexcept;
  void dprint(const char* name, const char* fmt, ...) noexcept;
  void mprint(const char* fmt, ...) noexcept;
  void start_evaluation() noexcept;
  void process_evaluation(size_t length) noexcept;
  void report_evaluation() noexcept;
  bool check_count() noexcept;

private:
  size_t byte;        // sent bytes
  size_t packet;      // sent packets
  double kbps;        // kilo byte per second
  double kpps;        // kilo packet per second
  clock_t time_start; // start time
  clock_t time_now;   // current time
  double time_diff;   // time difference
  InputType type;     // input type
};

#endif

// vim: et:ts=2:sw=2
