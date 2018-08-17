#ifndef OPTIONS_H
#define OPTIONS_H

#include <ctime>
#include <fstream>
#include <string>

#define F __func__

enum class InputType {
  None, // no type
  File, // file name
  Nic,  // network interface name
};

struct Config {
  bool mode_debug;    // debug mode (print debug messages)
  bool mode_eval;     // evaluation mode (report statistics)
  bool mode_kafka;    // do not send data to kafka (parse packet only)
  bool mode_parse;    // do not parse packet (send hardcoded sample data)
  size_t count_send;  // send packet count
  size_t count_skip;  // skip packet count
  size_t count_queue; // queue packet count (how many packet send once)
  std::string input;  // input pcapfile or nic
  std::string output; // output file
  std::string filter; // tcpdump filter string
  std::string broker; // kafka broker
  std::string topic;  // kafka topic
  Config()
      : mode_debug(false), mode_eval(false), mode_kafka(false),
        mode_parse(false), count_send(0), count_skip(0), count_queue(0)
  {
  }
};

class Options {
public:
  Config conf;
  Options() = default;
  Options(const Config&);
  Options(const Options&);
  Options& operator=(const Options&);
  Options(Options&&) = delete;
  Options& operator=(Options&&) = delete;
  ~Options();
  void show_options() noexcept;
  void dprint(const char* name, const char* fmt, ...) noexcept;
  void eprint(const char* name, const char* fmt, ...) noexcept;
  void mprint(const char* fmt, ...) noexcept;
  void fprint(const char* message) noexcept;
  bool check_count() noexcept;
  void start_evaluation() noexcept;
  void process_evaluation(size_t length) noexcept;
  void report_evaluation() noexcept;
  bool open_output_file() noexcept;

private:
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

#endif

// vim: et:ts=2:sw=2
