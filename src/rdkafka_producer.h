#ifndef RDKAFKA_PRODUCER_H
#define RDKAFKA_PRODUCER_H

#include <string>

#include "librdkafka/rdkafkacpp.h"

class Rdkafka_producer {

public:
  Rdkafka_producer() = delete;

  Rdkafka_producer(const std::string& brokers, const std::string& topic);
  Rdkafka_producer(const Rdkafka_producer&) = delete;
  Rdkafka_producer& operator=(const Rdkafka_producer&) = delete;
  Rdkafka_producer(Rdkafka_producer&&) noexcept;
  Rdkafka_producer& operator=(const Rdkafka_producer&&) = delete;
  ~Rdkafka_producer();
  bool produce(const std::string& message);

private:
  RdKafka::Conf* conf;
  RdKafka::Conf* tconf;
  RdKafka::Topic* topic;
  RdKafka::Producer* producer;
  int32_t partition = RdKafka::Topic::PARTITION_UA;
  std::string errstr;
};

#endif
