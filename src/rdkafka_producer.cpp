#include <array>
#include <iostream>

#include "rdkafka_producer.h"

using namespace std;

enum class KafkaConfType {
  GLOBAL = 1,
  TOPIC,
};

struct KafkaConf {
  const KafkaConfType type;
  const char* key;
  const char* value;
  const char* min;
  const char* max;
  const char* base;
  const char* desc;
};

static const array<KafkaConf, 4> kafka_conf = {{
    {KafkaConfType::GLOBAL, "delivery.report.only.error", "true", "false",
     "true", "false", "Only provide delivery reports for failed messages"},

    // FIXME: we need test
    {KafkaConfType::GLOBAL, "message.max.bytes", "1000000", "1000",
     "1000000000", "1000000", "Maximum Kafka protocol request message size"},
    {KafkaConfType::GLOBAL, "queue.buffering.max.messages", "100000", "1",
     "10000000", "100000",
     "Maximum total message size sum allowed on the producer queue"},
    {KafkaConfType::GLOBAL, "queue.buffering.max.kbytes", "1048576", "1",
     "2097151", "1048576",
     "Maximum total message size sum allowed on the producer queue"},

#if 0
    // it will reduce performance
    {KafkaConfType::TOPIC, "compression.codec", "l24", "none", "none", "none",
     "none/gzip/snappy/lz4"},
#endif
}};

void RdDeliveryReportCb::dr_cb(RdKafka::Message& message)
{
  opt.eprint(F, "message delivery(", message.len(),
             " bytes): ", message.errstr());

  if (message.key()) {
    opt.eprint(F, "key: ", *(message.key()));
  }
}

void RdEventCb::event_cb(RdKafka::Event& event)
{
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    opt.eprint(F, RdKafka::err2str(event.err()), ": ", event.str());
    if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN) {
      // FIXME: what do we do?
    }
    break;
  case RdKafka::Event::EVENT_STATS:
    opt.dprint(F, "STAT: ", event.str());
    break;
  case RdKafka::Event::EVENT_LOG:
    opt.dprint(F, "LOG-", event.severity(), "-", event.fac(), ": ",
               event.str());
    break;
  default:
    opt.eprint(F, "EVENT: ", RdKafka::err2str(event.err()), ": ", event.str());
    break;
  }
}

RdkafkaProducer::RdkafkaProducer(const Options& _opt) : opt(_opt)
{
  if (opt.conf.broker.empty() || opt.conf.topic.empty()) {
    throw runtime_error("Invalid constructor parameter");
  }

  rd_dr_cb.opt = _opt;
  rd_event_cb.opt = _opt;

  // Create configuration objects
  conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  if (!conf) {
    throw runtime_error("Failed to create global configuration object");
  }
  tconf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  if (!tconf) {
    throw runtime_error("Failed to create topic configuration object");
  }

  set_kafka_conf();
  show_kafka_conf();

  string errstr;

  // Create producer handle
  producer.reset(RdKafka::Producer::create(conf.get(), errstr));
  if (!producer) {
    throw runtime_error("Failed to create producer: " + errstr);
  }

  // Create topic handle
  topic.reset(RdKafka::Topic::create(producer.get(), opt.conf.topic,
                                     tconf.get(), errstr));
  if (!topic) {
    throw runtime_error("Failed to create topic: " + errstr);
  }
}

void RdkafkaProducer::set_kafka_conf()
{
  string errstr;

  // Set configuration properties
  if (conf->set("metadata.broker.list", opt.conf.broker, errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: metadata.broker.list: " +
                        errstr);
  }
  if (conf->set("event_cb", &rd_event_cb, errstr) != RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: event_cb: " + errstr);
  }
  if (conf->set("dr_cb", &rd_dr_cb, errstr) != RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: dr_cb: " + errstr);
  }

  // Set configuration properties: optional features
  for (const auto& entry : kafka_conf) {
    opt.dprint(F, "kafka_conf: ", entry.key, "=", entry.value, " (", entry.min,
               "~", entry.max, ", ", entry.base, ")");
    switch (entry.type) {
    case KafkaConfType::GLOBAL:
      if (conf->set(entry.key, entry.value, errstr) != RdKafka::Conf::CONF_OK) {
        throw runtime_error("Failed to set global config: " +
                            string(entry.key) + ": " + errstr);
      }
      break;
    case KafkaConfType::TOPIC:
      if (tconf->set(entry.key, entry.value, errstr) !=
          RdKafka::Conf::CONF_OK) {
        throw runtime_error("Failed to set topic config: " + string(entry.key) +
                            ": " + errstr);
      }
      break;
    default:
      opt.eprint(F,
                 "unknown kafka config type: ", static_cast<int>(entry.type));
      break;
    }
  }
}

void RdkafkaProducer::show_kafka_conf() const
{
  // show kafka producer config
  if (opt.conf.mode_debug) {
    int pass;
    for (pass = 0; pass < 2; pass++) {
      list<string>* dump;
      if (pass == 0) {
        dump = conf->dump();
        opt.dprint(F, "### Global Config");
      } else {
        dump = tconf->dump();
        opt.dprint(F, "### Topic Config");
      }
      // FIXME: iterator
      for (auto it = dump->cbegin(); it != dump->cend();) {
        string key = *it++;
        string value = *it++;
        opt.dprint(F, key, "=", value);
      }
    }
  }
}

void RdkafkaProducer::wait_queue(const int count) noexcept
{
  while (producer->outq_len() > count) {
    opt.dprint(F, "waiting for ", producer->outq_len());

    // FIXME: test effects of timeout (original 1000)
    producer->poll(100);
  }
}

bool RdkafkaProducer::produce_core(const string& message) noexcept
{
  // Produce message
  RdKafka::ErrorCode resp = producer->produce(
      topic.get(), RdKafka::Topic::PARTITION_UA,
      RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      const_cast<char*>(message.c_str()), message.size(), nullptr, nullptr);

  if (resp != RdKafka::ERR_NO_ERROR) {
    opt.eprint(F, "produce failed: ", RdKafka::err2str(resp));
    return false;
  }

  opt.dprint(F, "produced message: ", message.size(), " bytes");

  producer->poll(0);

  return true;
}

bool RdkafkaProducer::produce(const string& message) noexcept
{
  if (queue_data.length() + message.length() >= opt.conf.queue_size) {
    if (!produce_core(queue_data)) {
      // FIXME: error handling
    }
    queue_data.clear();

    wait_queue(99999);
  }
  queue_data += message + '\n';

  return true;
}

RdkafkaProducer::~RdkafkaProducer()
{
  opt.dprint(F, "last queued data: ", queue_data.size());

  if (queue_data.size()) {
    produce_core(queue_data);
    queue_data.clear();
  }
  wait_queue(0);

  RdKafka::wait_destroyed(1000);
}

// vim: et:ts=2:sw=2
