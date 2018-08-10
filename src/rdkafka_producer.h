#ifndef RDKAFKA_PRODUCER_H
#define RDKAFKA_PRODUCER_H

#include <memory>
#include <string>

#include "librdkafka/rdkafkacpp.h"
#include "options.h"

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

class Rdkafka_producer {
public:
  Rdkafka_producer() = delete;
  Rdkafka_producer(const Options&);
  Rdkafka_producer(const Rdkafka_producer&) = delete;
  Rdkafka_producer& operator=(const Rdkafka_producer&) = delete;
  Rdkafka_producer(Rdkafka_producer&&) = default;
  Rdkafka_producer& operator=(Rdkafka_producer&&) = default;
  ~Rdkafka_producer();
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
  size_t queue_count;
  bool produce_core(const std::string& message) noexcept;
};

#endif

// vim: et:ts=2:sw=2
