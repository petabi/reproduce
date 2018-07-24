#ifndef RDKAFKA_PRODUCER_H
#define RDKAFKA_PRODUCER_H
#include "rdkafka.h"
#include <iostream>
#include <string>

class Rdkafka_producer {

public:
  ~Rdkafka_producer();
  bool server_conf(const std::string& brokers, const std::string& topic);
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
