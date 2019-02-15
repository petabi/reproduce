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
  ~Report();

  void start(const size_t id) noexcept;
  void process(const size_t bytes) noexcept;
  void skip(const size_t bytes) noexcept;
  void end(const size_t id) noexcept;
  std::shared_ptr<Config> conf;

private:
  std::ofstream report_file;
  size_t start_id{0};
  size_t end_id{0};
  size_t sum_bytes{0};
  size_t min_bytes{0};
  size_t max_bytes{0};
  double avg_bytes{0.0};
  size_t skip_bytes{0};
  size_t skip_cnt{0};
  size_t process_cnt{0};
  size_t total_cnt{0};
  std::chrono::time_point<std::chrono::steady_clock> time_start{
      (std::chrono::milliseconds::zero())};
  std::chrono::time_point<std::chrono::steady_clock> time_now{
      (std::chrono::milliseconds::zero())};
  std::chrono::duration<double> time_diff{0.0};
};

#endif

// vim: et:ts=2:sw=2
