#ifndef REPORT_H
#define REPORT_H

#include <memory>

#include "config.h"

class Report {
public:
  Report() = delete;
  Report(std::shared_ptr<Config>);
  Report(const Report&) = delete;
  Report& operator=(const Report&) = delete;
  Report(Report&&) = delete;
  Report& operator=(Report&&) = delete;
  ~Report() = default;
  void start() noexcept;
  void calculate() noexcept;
  void process(const size_t orig_length, const size_t sent_length) noexcept;
  void end() noexcept;
  void fail() noexcept;
  size_t get_sent_count() const noexcept;
  std::shared_ptr<Config> conf;

private:
  size_t orig_byte{0};
  size_t orig_byte_min{0};
  size_t orig_byte_max{0};
  double orig_byte_avg{0.0};
  size_t sent_byte{0};
  size_t sent_byte_min{0};
  size_t sent_byte_max{0};
  double sent_byte_avg{0.0};
  size_t sent_count{0};
  size_t fail_count{0};
  double perf_kbps{0.0};
  double perf_kpps{0.0};
  clock_t time_start{0};
  clock_t time_prev{0};
  clock_t time_now{0};
  double time_diff{0.0};
};

#endif

// vim: et:ts=2:sw=2
