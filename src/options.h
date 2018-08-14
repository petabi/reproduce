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
  bool mode_debug;    // debug mode (print debug messages)
  bool mode_eval;     // evaluation mode (report statistics)
  bool mode_kafka;    // do not send data to kafka (parse packet only)
  bool mode_parse;    // do not parse packet (send hardcoded sample data)
  bool mode_write_output;     // write output file
  size_t count_send;  // send packet count
  size_t count_skip;  // skip packet count
  size_t count_queue; // queue packet count (how many packet send once)
  std::string input;  // input pcapfile or nic
  std::string output; // output file
  std::string filter; // tcpdump filter string
  std::string broker; // kafka broker
  std::string topic;  // kafka topic

  Options();
  Options(const Options&) = default;
  Options& operator=(const Options&) = default;
  Options(Options&&) = delete;
  Options& operator=(Options&&) = delete;
  ~Options();
  void show_options() noexcept;
  void dprint(const char* name, const char* fmt, ...) noexcept;
  void eprint(const char* name, const char* fmt, ...) noexcept;
  void mprint(const char* fmt, ...) noexcept;
  void fprint(std::ofstream& stream, const char* message) noexcept;
  bool check_count() noexcept;
  void start_evaluation() noexcept;
  void process_evaluation(size_t length) noexcept;
  void report_evaluation() noexcept;

private:
  size_t sent_byte;     // sent bytes
  size_t sent_packet;   // sent packets
  double perf_kbps;     // kilo byte per second
  double perf_kpps;     // kilo packet per second
  clock_t time_start;   // start time
  clock_t time_now;     // current time
  double time_diff;     // time difference
  InputType input_type; // input type
};

#endif

// vim: et:ts=2:sw=2
