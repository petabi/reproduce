#ifndef PRODUCER_H
#define PRODUCER_H

#include <iostream>
#include <memory>
#include <string>

#include "librdkafka/rdkafkacpp.h"

#include "options.h"

/**
 * Producer
 */

enum class OutputType {
  KAFKA,
  FILE,
  NONE,
};

class Producer {
public:
  virtual bool produce(const std::string& message) = 0;
};

/**
 * KafkaProducer
 */

class RdDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
  Options opt;
  void dr_cb(RdKafka::Message&) override;
};

class RdEventCb : public RdKafka::EventCb {
public:
  Options opt;
  void event_cb(RdKafka::Event&) override;
};

class KafkaProducer : Producer {
public:
  KafkaProducer() = delete;
  KafkaProducer(const Options&);
  KafkaProducer(const KafkaProducer&) = delete;
  KafkaProducer& operator=(const KafkaProducer&) = delete;
  KafkaProducer(KafkaProducer&&) = default;
  KafkaProducer& operator=(KafkaProducer&&) = default;
  ~KafkaProducer();
  bool produce(const std::string& message) noexcept override;

private:
  Options opt;
  std::unique_ptr<RdKafka::Conf> conf;
  std::unique_ptr<RdKafka::Conf> tconf;
  std::unique_ptr<RdKafka::Topic> topic;
  std::unique_ptr<RdKafka::Producer> producer;
  RdDeliveryReportCb rd_dr_cb;
  RdEventCb rd_event_cb;
  std::string queue_data;
  bool produce_core(const std::string& message) noexcept;
  void wait_queue(const int count) noexcept;
  void set_kafka_conf();
  void show_kafka_conf() const;
};

/**
 * FileProducer
 */

class FileProducer : Producer {
public:
  FileProducer() = delete;
  FileProducer(Config);
  FileProducer(const FileProducer&) = delete;
  FileProducer& operator=(const FileProducer&) = delete;
  FileProducer(FileProducer&&) = delete;
  FileProducer& operator=(FileProducer&&) = delete;
  ~FileProducer();
  bool produce(const std::string& message) noexcept override;

private:
  Config conf;
  std::ofstream file;
  bool open() noexcept;
};

/**
 * NullProducer
 */

class NullProducer : Producer {
public:
  NullProducer() = delete;
  NullProducer(const NullProducer&) = delete;
  NullProducer& operator=(const NullProducer&) = delete;
  NullProducer(NullProducer&&) = delete;
  NullProducer& operator=(NullProducer&&) = delete;
  ~NullProducer();
  bool produce(const std::string& message) noexcept override;
};

#endif

// vim: et:ts=2:sw=2
