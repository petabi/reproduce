#ifndef RDKAFKA_PRODUCER_H
#define RDKAFKA_PRODUCER_H

#include <string>

#include <librdkafka/rdkafka.h>

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
  static void dr_msg_cb(rd_kafka_t* rk, const rd_kafka_message_t* rkmessage,
                        void* opaque);
  rd_kafka_t* rk;        /* Producer instance handle */
  rd_kafka_topic_t* rkt; /* Topic object */
  rd_kafka_conf_t* conf; /* Temporary configuration object */
  std::string errstr;
};

#endif
