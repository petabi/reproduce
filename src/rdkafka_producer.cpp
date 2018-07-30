#include <cstdio>
#include <iostream>

#include "rdkafka_producer.h"

Rdkafka_producer::Rdkafka_producer(const std::string& brokers,
                                   const std::string& topic)
{
  char errstr[512]; /* librdkafka API error reporting buffer */
  conf = rd_kafka_conf_new();
  /* producer config */
  if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
    rd_kafka_conf_destroy(conf);
    throw std::runtime_error(errstr);
  }
  // TODO : optimize rd_kafka_conf_set config parameters
  // rd_kafka_conf_set(conf, "queue.buffering.max.messages", "1000000", nullptr,
  //                  0);
  // rd_kafka_conf_set(conf, "queue.buffering.max.kbytes", "2000000", nullptr,
  // 0);
  rd_kafka_conf_set(conf, "message.send.max.retries", "3", nullptr, 0);

  rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
  rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!rk) {
    rd_kafka_conf_destroy(conf);
    throw std::runtime_error(std::string("%% Failed to create new producer:") +
                             errstr);
  }

  /* Create topic object that will be reused for each message
   * produced.
   *
   * Both the producer instance (rd_kafka_t) and topic objects (topic_t)
   * are long-lived objects that should be reused as much as possible.
   */
  rkt = rd_kafka_topic_new(rk, topic.c_str(), nullptr);
  if (!rkt) {
    rd_kafka_conf_destroy(conf);
    rd_kafka_destroy(rk);
    throw std::runtime_error(std::string("%% Failed to create topic object: ") +
                             rd_kafka_err2str(rd_kafka_last_error()));
  }
}

Rdkafka_producer::Rdkafka_producer(Rdkafka_producer&& other) noexcept
{
  std::cout << "%% Flushing final messages..\n";
  rd_kafka_flush(rk, 10 * 1000 /* wait for max 10 seconds */);
  rd_kafka_t* other_rk = other.rk;
  rd_kafka_topic_t* other_rkt = other.rkt;
  rd_kafka_conf_t* other_conf = other.conf;
  other.rk = nullptr;
  other.rkt = nullptr;
  other.conf = nullptr;
  if (conf != nullptr)
    rd_kafka_conf_destroy(conf);
  if (rk != nullptr) {
    rd_kafka_destroy(rk);
  }
  if (rkt != nullptr) {
    rd_kafka_topic_destroy(rkt);
  }
  conf = other_conf;
  rk = other_rk;
  rkt = other_rkt;
}

bool Rdkafka_producer::produce(const std::string& message)
{

  bool stop = false;
  size_t msg_len = message.length();
  if (msg_len == 0)
    return true;

  while (stop == false) {
    if (rd_kafka_produce(
            /* Topic object */
            rkt,
            /* Use builtin partitioner to select partition*/
            RD_KAFKA_PARTITION_UA,
            /* Make a copy of the payload. */
            RD_KAFKA_MSG_F_COPY,
            /* Message payload (value) and length */
            (void*)message.c_str(), msg_len,
            /* Optional key and its length */
            nullptr, 0,
            /* Message opaque, provided in
             * delivery report callback as
             * msg_opaque. */
            nullptr) == -1) {
      /**
       * Failed to *enqueue* message for producing.
       */
      std::cerr << "%% Failed to produce to topic " << rd_kafka_topic_name(rkt)
                << " : " << rd_kafka_err2str(rd_kafka_last_error()) << '\n';

      /* Poll to handle delivery reports */
      if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
        /* If the internal queue is full, wait for
         * messages to be delivered and then retry.
         * The internal queue represents both
         * messages to be sent and messages that have
         * been sent or failed, awaiting their
         * delivery report callback to be called.
         *
         * The internal queue is limited by the
         * configuration property
         * queue.buffering.max.messages */
        rd_kafka_poll(rk, 1000 /*block for max 1000ms*/);
      }
    } else {
      std::cout << "%% Enqueued message (" << msg_len << " bytes) for topic "
                << rd_kafka_topic_name(rkt) << '\n';
      stop = true;
    }
  }

  /* A producer application should continually serve
   * the delivery report queue by calling rd_kafka_poll()
   * at frequent intervals.
   * Either put the poll call in your main loop, or in a
   * dedicated thread, or call it after every
   * rd_kafka_produce() call.
   * Just make sure that rd_kafka_poll() is still called
   * during periods where you are not producing any messages
   * to make sure previously produced messages have their
   * delivery report callback served (and any other callbacks
   * you register). */
  rd_kafka_poll(rk, 0 /*non-blocking*/);

  /* Wait for final messages to be delivered or fail.
   * rd_kafka_flush() is an abstraction over rd_kafka_poll() which
   * waits for all messages to be delivered. */
  return true;
}

void Rdkafka_producer::dr_msg_cb(rd_kafka_t* rk,
                                 const rd_kafka_message_t* rkmessage,
                                 void* opaque)
{
  if (rkmessage->err)
    std::cerr << "%% Message delivery failed: "
              << rd_kafka_err2str(rkmessage->err) << '\n';
  else
    std::cout << "%% Message delivered (" << rkmessage->len
              << " bytes, partition " << rkmessage->partition << '\n';
}

Rdkafka_producer::~Rdkafka_producer()
{

  std::cout << "%% Flushing final messages..\n";
  // rd_kafka_flush(rk, 10 * 1000 /* wait for max 10 seconds */);
  if (conf != nullptr)
    rd_kafka_conf_destroy(conf);
  if (rk != nullptr)
    rd_kafka_destroy(rk);
  if (rkt != nullptr)
    rd_kafka_topic_destroy(rkt);
}
