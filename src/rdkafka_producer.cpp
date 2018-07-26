#include <cstdio>

#include "rdkafka_producer.h"

bool Rdkafka_producer::server_conf(const std::string& brokers,
                                   const std::string& topic)
{
  char errstr[512]; /* librdkafka API error reporting buffer */
  conf = rd_kafka_conf_new();
  /* producer config */
  if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK) {
    fprintf(stdout, "%s\n", errstr);
    return false;
  }
  // TODO : optimize rd_kafka_conf_set config parameters
  rd_kafka_conf_set(conf, "queue.buffering.max.messages", "1000000", nullptr,
                    0);
  rd_kafka_conf_set(conf, "queue.buffering.max.kbytes", "2000000", nullptr, 0);
  rd_kafka_conf_set(conf, "message.send.max.retries", "3", nullptr, 0);

  rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
  rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!rk) {
    fprintf(stdout, "%% Failed to create new producer: %s\n", errstr);
    return false;
  }

  /* Create topic object that will be reused for each message
   * produced.
   *
   * Both the producer instance (rd_kafka_t) and topic objects (topic_t)
   * are long-lived objects that should be reused as much as possible.
   */
  rkt = rd_kafka_topic_new(rk, topic.c_str(), nullptr);
  if (!rkt) {
    fprintf(stdout, "%% Failed to create topic object: %s\n",
            rd_kafka_err2str(rd_kafka_last_error()));
    rd_kafka_destroy(rk);
    return false;
  }
  return true;
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
      fprintf(stdout, "%% Failed to produce to topic %s: %s\n",
              rd_kafka_topic_name(rkt),
              rd_kafka_err2str(rd_kafka_last_error()));

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
      fprintf(stdout,
              "%% Enqueued message (%zd bytes) "
              "for topic %s\n",
              msg_len, rd_kafka_topic_name(rkt));
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
    fprintf(stdout, "%% Message delivery failed: %s\n",
            rd_kafka_err2str(rkmessage->err));
  else
    fprintf(stdout,
            "%% Message delivered (%zd bytes, "
            "partition %" PRId32 ")\n",
            rkmessage->len, rkmessage->partition);
}

Rdkafka_producer::~Rdkafka_producer()
{

  fprintf(stdout, "%% Flushing final messages..\n");
  rd_kafka_flush(rk, 10 * 1000 /* wait for max 10 seconds */);
  /* Destroy topic object */
  rd_kafka_topic_destroy(rkt);

  /* Destroy the producer instance */
  rd_kafka_destroy(rk);
}
