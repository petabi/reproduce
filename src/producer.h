#ifndef PRODUCER_H
#define PRODUCER_H

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
};

/**
 * KafkaProducer
 */

class RdDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message&) override;
};

class RdEventCb : public RdKafka::EventCb {
public:
  void event_cb(RdKafka::Event&) override;
};

class KafkaProducer : public Producer {
public:
  KafkaProducer() = delete;
  KafkaProducer(Config);
  KafkaProducer(const KafkaProducer&) = delete;
  KafkaProducer& operator=(const KafkaProducer&) = delete;
  KafkaProducer(KafkaProducer&&) = delete;
  KafkaProducer& operator=(KafkaProducer&&) = delete;
  ~KafkaProducer();
  bool produce(const std::string& message) noexcept override;

private:
  Config conf;
  std::unique_ptr<RdKafka::Conf> kafka_gconf;
  std::unique_ptr<RdKafka::Conf> kafka_tconf;
  std::unique_ptr<RdKafka::Topic> kafka_topic;
  std::unique_ptr<RdKafka::Producer> kafka_producer;
  RdDeliveryReportCb rd_dr_cb;
  RdEventCb rd_event_cb;
  std::string queue_data;
  size_t convert_queue_threshold{0};
  bool produce_core(const std::string& message) noexcept;
  void wait_queue(const int count) noexcept;
  void set_kafka_conf();
  void set_kafka_conf_file(const std::string& conf_file);
  void set_kafka_threshold();
  void show_kafka_conf() const;
};

/**
 * FileProducer
 */

class FileProducer : public Producer {
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

class NullProducer : public Producer {
public:
  NullProducer() = delete;
  NullProducer(Config);
  NullProducer(const NullProducer&) = delete;
  NullProducer& operator=(const NullProducer&) = delete;
  NullProducer(NullProducer&&) = delete;
  NullProducer& operator=(NullProducer&&) = delete;
  ~NullProducer() = default;
  bool produce(const std::string& message) noexcept override;

private:
  Config conf;
};

#endif

// vim: et:ts=2:sw=2
