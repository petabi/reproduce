#ifndef RDKAFKA_PRODUCER_H
#define RDKAFKA_PRODUCER_H

#include <memory>
#include <string>

#include "librdkafka/rdkafkacpp.h"

class RdDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message& message) override;
};

class RdEventCb : public RdKafka::EventCb {
public:
  void event_cb(RdKafka::Event& event) override;
};

class Rdkafka_producer {
public:
  Rdkafka_producer() = delete;
  Rdkafka_producer(const std::string& brokers, const std::string& topic);
  Rdkafka_producer(const Rdkafka_producer&) = delete;
  Rdkafka_producer& operator=(const Rdkafka_producer&) = delete;
  Rdkafka_producer(Rdkafka_producer&&) = default;
  Rdkafka_producer& operator=(Rdkafka_producer&&) = default;
  ~Rdkafka_producer();
  bool produce(const std::string& message);

private:
  std::unique_ptr<RdKafka::Conf> conf;
  std::unique_ptr<RdKafka::Conf> tconf;
  std::unique_ptr<RdKafka::Topic> topic;
  std::unique_ptr<RdKafka::Producer> producer;
  RdEventCb rd_event_cb;
  RdDeliveryReportCb rd_dr_cb;
};

#endif

// vim: et:ts=2:sw=2
