#ifndef PRODUCER_H
#define PRODUCER_H

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "config.h"
#include "util.h"

struct InnerProducer;

/**
 * Producer
 */

static constexpr size_t default_produce_max_bytes = 100000;

class Producer {
public:
  virtual auto produce(const char* message, size_t len, bool flush = false)
      -> bool = 0;
  virtual ~Producer() = 0;
  [[nodiscard]] virtual auto get_max_bytes() const -> size_t = 0;
};

/**
 * KafkaProducer
 */
struct produce_ack_cnt {
  size_t produce_cnt = 0;
  size_t ack_cnt = 0;
};

class KafkaProducer : public Producer {

public:
  KafkaProducer() = delete;
  KafkaProducer(std::shared_ptr<Config>);
  KafkaProducer(const KafkaProducer&) = delete;
  auto operator=(const KafkaProducer&) -> KafkaProducer& = delete;
  KafkaProducer(KafkaProducer&&) = delete;
  auto operator=(KafkaProducer &&) -> KafkaProducer& = delete;
  ~KafkaProducer() override;
  auto produce(const char* message, size_t len, bool flush = false) noexcept
      -> bool override;
  [[nodiscard]] auto get_max_bytes() const noexcept -> size_t override;

private:
  InnerProducer* inner{nullptr};
  produce_ack_cnt pac;
  std::shared_ptr<Config> conf;
  std::map<std::string, std::string> kafka_gconf;
  std::map<std::string, std::string> kafka_tconf;
  std::string queue_data;
  size_t queue_data_cnt{0};
  bool period_chk{false};
  std::chrono::time_point<std::chrono::steady_clock> last_time{
      (std::chrono::milliseconds::zero())};
  std::chrono::time_point<std::chrono::steady_clock> current_time{
      (std::chrono::milliseconds::zero())};
  std::chrono::duration<double> time_diff{0.0};
  auto produce_core(const std::string& message) noexcept -> bool;
  void set_kafka_conf();
  void set_kafka_conf_file(const std::string& conf_file);
  void show_kafka_conf() const;
  auto period_queue_flush() noexcept -> bool;
};

/**
 * FileProducer
 */

class FileProducer : public Producer {
public:
  FileProducer() = delete;
  FileProducer(std::shared_ptr<Config>);
  FileProducer(const FileProducer&) = delete;
  auto operator=(const FileProducer&) -> FileProducer& = delete;
  FileProducer(FileProducer&&) = delete;
  auto operator=(FileProducer &&) -> FileProducer& = delete;
  ~FileProducer() override;
  auto produce(const char* message, size_t len, bool flush = false) noexcept
      -> bool override;
  auto get_max_bytes() const noexcept -> size_t override;

private:
  std::shared_ptr<Config> conf;
  std::ofstream file;
  auto open() noexcept -> bool;
};

/**
 * NullProducer
 */

class NullProducer : public Producer {
public:
  NullProducer() = delete;
  NullProducer(std::shared_ptr<Config>);
  NullProducer(const NullProducer&) = delete;
  auto operator=(const NullProducer&) -> NullProducer& = delete;
  NullProducer(NullProducer&&) = delete;
  auto operator=(NullProducer &&) -> NullProducer& = delete;
  ~NullProducer() override;
  auto produce(const char* message, size_t len, bool flush = false) noexcept
      -> bool override;
  [[nodiscard]] auto get_max_bytes() const noexcept -> size_t override;

private:
  std::shared_ptr<Config> conf;
};

#endif

// vim: et:ts=2:sw=2
