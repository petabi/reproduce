#ifndef PRODUCER_H
#define PRODUCER_H

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "librdkafka/rdkafkacpp.h"

#include "config.h"
#include "util.h"

/**
 * Producer
 */

class Producer {
public:
  virtual bool produce(const std::string& message) = 0;
  virtual ~Producer() = 0;
};

/**
 * KafkaProducer
 */

class RdDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
  static bool error;
  void dr_cb(RdKafka::Message& message) override;
};

class RdEventCb : public RdKafka::EventCb {
public:
  static bool error;
  void event_cb(RdKafka::Event& event) override;
};

class KafkaProducer : public Producer {
public:
  KafkaProducer() = delete;
  KafkaProducer(std::shared_ptr<Config>);
  KafkaProducer(const KafkaProducer&) = delete;
  KafkaProducer& operator=(const KafkaProducer&) = delete;
  KafkaProducer(KafkaProducer&&) = delete;
  KafkaProducer& operator=(KafkaProducer&&) = delete;
  ~KafkaProducer() override;
  bool produce(const std::string& message) noexcept override;

private:
  std::shared_ptr<Config> conf;
  std::unique_ptr<RdKafka::Conf> kafka_gconf;
  std::unique_ptr<RdKafka::Conf> kafka_tconf;
  std::unique_ptr<RdKafka::Topic> kafka_topic;
  std::unique_ptr<RdKafka::Producer> kafka_producer;
  RdDeliveryReportCb rd_dr_cb;
  RdEventCb rd_event_cb;
  std::string queue_data;
  size_t queue_threshold{0};
  bool queue_auto_flush{false};
  std::chrono::time_point<std::chrono::steady_clock> last_time{
      (std::chrono::milliseconds::zero())};
  std::chrono::time_point<std::chrono::steady_clock> current_time{
      (std::chrono::milliseconds::zero())};
  std::chrono::duration<double> time_diff{0.0};
  bool produce_core(const std::string& message) noexcept;
  void wait_queue(const int count) noexcept;
  void set_kafka_conf();
  void set_kafka_conf_file(const std::string& conf_file);
  void set_kafka_threshold();
  void show_kafka_conf() const;
  bool period_queue_flush() noexcept;
};

/**
 * FileProducer
 */

class FileProducer : public Producer {
public:
  FileProducer() = delete;
  FileProducer(std::shared_ptr<Config>);
  FileProducer(const FileProducer&) = delete;
  FileProducer& operator=(const FileProducer&) = delete;
  FileProducer(FileProducer&&) = delete;
  FileProducer& operator=(FileProducer&&) = delete;
  ~FileProducer() override;
  bool produce(const std::string& message) noexcept override;

private:
  std::shared_ptr<Config> conf;
  std::ofstream file;
  bool open() noexcept;
};

/**
 * NullProducer
 */

class NullProducer : public Producer {
public:
  NullProducer() = delete;
  NullProducer(std::shared_ptr<Config>);
  NullProducer(const NullProducer&) = delete;
  NullProducer& operator=(const NullProducer&) = delete;
  NullProducer(NullProducer&&) = delete;
  NullProducer& operator=(NullProducer&&) = delete;
  ~NullProducer() override;
  bool produce(const std::string& message) noexcept override;

private:
  std::shared_ptr<Config> conf;
};

#endif

// vim: et:ts=2:sw=2
