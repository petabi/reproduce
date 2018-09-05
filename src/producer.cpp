#include <array>

#include "producer.h"

using namespace std;

/**
 * KafkaProducer
 */

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
     "Maximum number of messages allowed on the producer queue"},
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
  Util::eprint(F, "message delivery(", message.len(),
               " bytes): ", message.errstr());

  if (message.key()) {
    Util::eprint(F, "key: ", *(message.key()));
  }
}

void RdEventCb::event_cb(RdKafka::Event& event)
{
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    Util::eprint(F, RdKafka::err2str(event.err()), ": ", event.str());
    if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN) {
      // FIXME: what do we do?
    }
    break;
  case RdKafka::Event::EVENT_STATS:
    Util::dprint(F, "STAT: ", event.str());
    break;
  case RdKafka::Event::EVENT_LOG:
    Util::dprint(F, "LOG-", event.severity(), "-", event.fac(), ": ",
                 event.str());
    break;
  default:
    Util::eprint(F, "EVENT: ", RdKafka::err2str(event.err()), ": ",
                 event.str());
    break;
  }
}

KafkaProducer::KafkaProducer(Config _conf) : conf(move(_conf))
{
  if (conf.kafka_broker.empty() || conf.kafka_topic.empty()) {
    throw runtime_error("Invalid constructor parameter");
  }

  // Create configuration objects
  kafka_gconf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  if (!kafka_gconf) {
    throw runtime_error("Failed to create global configuration object");
  }
  kafka_tconf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  if (!kafka_tconf) {
    throw runtime_error("Failed to create topic configuration object");
  }

  set_kafka_conf();
  show_kafka_conf();

  string errstr;

  // Create producer handle
  kafka_producer.reset(RdKafka::Producer::create(kafka_gconf.get(), errstr));
  if (!kafka_producer) {
    throw runtime_error("Failed to create kafka producer handle: " + errstr);
  }

  // Create topic handle
  kafka_topic.reset(RdKafka::Topic::create(
      kafka_producer.get(), conf.kafka_topic, kafka_tconf.get(), errstr));
  if (!kafka_topic) {
    throw runtime_error("Failed to create kafka topic handle: " + errstr);
  }
}

void KafkaProducer::set_kafka_conf()
{
  string errstr;

  // Set configuration properties
  if (kafka_gconf->set("metadata.broker.list", conf.kafka_broker, errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: metadata.broker.list: " +
                        errstr);
  }
  if (kafka_gconf->set("event_cb", &rd_event_cb, errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: event_cb: " + errstr);
  }
  if (kafka_gconf->set("dr_cb", &rd_dr_cb, errstr) != RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: dr_cb: " + errstr);
  }

  // Set configuration properties: optional features
  for (const auto& entry : kafka_conf) {
    Util::dprint(F, entry.key, "=", entry.value, " (", entry.min, "~",
                 entry.max, ", ", entry.base, ")");
    switch (entry.type) {
    case KafkaConfType::GLOBAL:
      if (kafka_gconf->set(entry.key, entry.value, errstr) !=
          RdKafka::Conf::CONF_OK) {
        throw runtime_error("Failed to set kafka global config: " +
                            string(entry.key) + ": " + errstr);
      }
      break;
    case KafkaConfType::TOPIC:
      if (kafka_tconf->set(entry.key, entry.value, errstr) !=
          RdKafka::Conf::CONF_OK) {
        throw runtime_error("Failed to set kafka topic config: " +
                            string(entry.key) + ": " + errstr);
      }
      break;
    default:
      Util::eprint(F,
                   "unknown kafka config type: ", static_cast<int>(entry.type));
      break;
    }
  }
}

void KafkaProducer::show_kafka_conf() const
{
  // show kafka producer config
  if (conf.mode_debug) {
    int pass;
    for (pass = 0; pass < 2; pass++) {
      list<string>* dump;
      if (pass == 0) {
        dump = kafka_gconf->dump();
        Util::dprint(F, "### Global Config");
      } else {
        dump = kafka_tconf->dump();
        Util::dprint(F, "### Topic Config");
      }
      for (auto it = dump->cbegin(); it != dump->cend();) {
        string key = *it++;
        string value = *it++;
        Util::dprint(F, key, "=", value);
      }
    }
  }
}

void KafkaProducer::wait_queue(const int count) noexcept
{
  while (kafka_producer->outq_len() > count) {
    Util::dprint(F, "waiting for ", kafka_producer->outq_len());

    // FIXME: test effects of timeout (original 1000)
    kafka_producer->poll(100);
  }
}

bool KafkaProducer::produce_core(const string& message) noexcept
{
  // Produce message
  RdKafka::ErrorCode resp = kafka_producer->produce(
      kafka_topic.get(), RdKafka::Topic::PARTITION_UA,
      RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      const_cast<char*>(message.c_str()), message.size(), nullptr, nullptr);

  if (resp != RdKafka::ERR_NO_ERROR) {
    Util::eprint(F, "produce failed: ", RdKafka::err2str(resp));
    return false;
  }

  Util::dprint(F, "produced message: ", message.size(), " bytes");

  kafka_producer->poll(0);

  return true;
}

bool KafkaProducer::produce(const string& message) noexcept
{
  if (queue_data.length() + message.length() >= conf.queue_size) {
    if (!produce_core(queue_data)) {
      // FIXME: error handling
    }
    queue_data.clear();

    wait_queue(99999);
  }
  queue_data += message + '\n';

  return true;
}

KafkaProducer::~KafkaProducer()
{
  Util::dprint(F, "last queued data: ", queue_data.size());

  if (queue_data.size()) {
    produce_core(queue_data);
    queue_data.clear();
  }
  wait_queue(0);

  RdKafka::wait_destroyed(1000);
}

/**
 * FileProducer
 */

FileProducer::FileProducer(Config _conf) : conf(move(_conf))
{
  if (!conf.output.empty()) {
    if (!open()) {
      throw runtime_error("Failed to open output file: " + conf.output);
    }
  }
}

FileProducer::~FileProducer()
{
  if (file.is_open()) {
    file.close();
  }
}

bool FileProducer::produce(const string& message) noexcept
{
  file << message << "\n";
  file.flush();
  // FIXME: check error?

  return true;
}

bool FileProducer::open() noexcept
{
  file.open(conf.output, ios::out);
  if (!file.is_open()) {
    Util::eprint(F, "Failed to open file: ", conf.output);
    return false;
  }

  return true;
}

/**
 * NullProducer
 */

NullProducer::NullProducer(Config _conf) : conf(move(_conf)) {}

bool NullProducer::produce(const string& message) noexcept { return true; }

// vim: et:ts=2:sw=2
