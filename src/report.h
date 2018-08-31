#ifndef REPORT_H
#define REPORT_H

#include "config.h"

class Report {
public:
  Config conf;
  Report() = delete;
  Report(Config&);
  Report(const Report&) = delete;
  Report& operator=(const Report&) = delete;
  Report(Report&&) = delete;
  Report& operator=(Report&&) = delete;
  ~Report() = default;
  void start() noexcept;
  void process(int length) noexcept;
  void end() const noexcept;
  void fail() noexcept;
  size_t get_sent_count() noexcept;

private:
  size_t sent_byte{0};
  size_t sent_count{0};
  size_t fail_count{0};
  double perf_kbps{0.0}; // kilo byte per sec
  double perf_kpps{0.0};
  clock_t time_start{0};
  clock_t time_now{0};
  double time_diff{0.0};
};

#endif

// vim: et:ts=2:sw=2
