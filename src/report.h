#ifndef REPORT_H
#define REPORT_H

#include <chrono>
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
  void process(const size_t bytes) noexcept;
  void end() noexcept;
  std::shared_ptr<Config> conf;

private:
  size_t sum_bytes{0};
  size_t min_bytes{0};
  size_t max_bytes{0};
  double avg_bytes{0.0};
  size_t count{0};
  double perf_kbps{0.0};
  std::chrono::time_point<std::chrono::steady_clock> time_start{
      (std::chrono::milliseconds::zero())};
  std::chrono::time_point<std::chrono::steady_clock> time_now{
      (std::chrono::milliseconds::zero())};
  std::chrono::duration<double> time_diff{0.0};
};

#endif

// vim: et:ts=2:sw=2
