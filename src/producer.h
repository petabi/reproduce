#ifndef PRODUCER_H
#define PRODUCER_H

#include <memory>
#include <string>

#include "librdkafka/rdkafkacpp.h"

#include "options.h"

/**
 * Producer
 */

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

class KafkaProducer {
public:
  KafkaProducer() = delete;
  KafkaProducer(const Options&);
  KafkaProducer(const KafkaProducer&) = delete;
  KafkaProducer& operator=(const KafkaProducer&) = delete;
  KafkaProducer(KafkaProducer&&) = default;
  KafkaProducer& operator=(KafkaProducer&&) = default;
  ~KafkaProducer();
  void wait_queue(const int count) noexcept;
  bool produce(const std::string& message) noexcept;

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
  void set_kafka_conf();
  void show_kafka_conf() const;
};

/**
 * FileProducer
 */

/**
 * NullProducer
 */

#endif

// vim: et:ts=2:sw=2
