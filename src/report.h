#ifndef REPORT_H
#define REPORT_H

#include "config.h"

class Report {
public:
  Report() = default;
  // Report(Config);
  Report(const Report&) = delete;
  Report& operator=(const Report&) = delete;
  Report(Report&&) = delete;
  Report& operator=(Report&&) = delete;
  ~Report() = default;
  void start() noexcept;
  void process(const int length) noexcept;
  void end() const noexcept;
  void fail() noexcept;
  size_t get_sent_count() const noexcept;
  Config conf;

private:
  size_t sent_byte{0};
  size_t sent_byte_min{0};
  size_t sent_byte_max{0};
  double sent_byte_avg{0.0};
  size_t sent_count{0};
  size_t fail_count{0};
  double perf_kbps{0.0};
  double perf_kpps{0.0};
  clock_t time_start{0};
  clock_t time_now{0};
  double time_diff{0.0};
  InputType input_type{InputType::NONE};
  OutputType output_type{InputType::NONE};
};

#endif

// vim: et:ts=2:sw=2
