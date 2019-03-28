#include <array>

#include "producer.h"

using namespace std;

/**
 * KafkaProducer
 */

enum class KafkaConfType {
  Global = 1,
  Topic,
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

static const array<KafkaConf, 5> kafka_conf = {{
    {KafkaConfType::Global, "delivery.report.only.error", "false", "false",
     "true", "false", "Only provide delivery reports for failed messages"},
    {KafkaConfType::Global, "message.max.bytes", "1000000", "1000",
     "1000000000", "1000000", "Maximum Kafka protocol request message size"},
    {KafkaConfType::Global, "queue.buffering.max.messages", "10000000", "1",
     "10000000", "100000",
     "Maximum number of messages allowed on the producer queue"},
    {KafkaConfType::Global, "queue.buffering.max.kbytes", "2097151", "1",
     "2097151", "1048576",
     "Maximum total message size sum allowed on the producer queue"},
    {KafkaConfType::Topic, "message.timeout.ms", "0", "0", "900000", "300000",
     "limit the time a produced message waits for successful delivery"},
#if 0
    // it will reduce performance
    {KafkaConfType::Topic, "compression.codec", "lz4", "none", "none", "none",
     "none/gzip/snappy/lz4"},
#endif
}};

Producer::~Producer() = default;

void RdDeliveryReportCb::dr_cb(RdKafka::Message& message)
{
// FIXME: error handling
#if 0
  switch (message.status()) {
  case RdKafka::Message::MSG_STATUS_NOT_PERSISTED:
    status_name = "NotPersisted";
    break;
  case RdKafka::Message::MSG_STATUS_POSSIBLY_PERSISTED:
    status_name = "PossiblyPersisted";
    break;
  case RdKafka::Message::MSG_STATUS_PERSISTED:
    status_name = "Persisted";
    break;
  default:
    status_name = "Unknown?";
    break;
  }
#endif
  auto* pacp = reinterpret_cast<produce_ack_cnt*>(message.msg_opaque());
  if (message.err() != 0) {
    Util::eprint("Message Produce Fail: ", message.errstr());
  } else {
    pacp->ack_cnt += 1;
    Util::iprint("Message Produce Success: ", message.len(),
                 " bytes, Total Produce ", pacp->ack_cnt, "/",
                 pacp->produce_cnt, "acked");
  }

  if (message.key()) {
    Util::eprint(", key: ", *(message.key()));
  }
}

void RdEventCb::event_cb(RdKafka::Event& event)
{
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    Util::eprint("Kafka Event Error: ", RdKafka::err2str(event.err()), ": ",
                 event.str());
    break;

// TODO : if necessary, add statistical function and throttle function.
#if 0
  case RdKafka::Event::EVENT_STATS:
    Util::iprint("Kafka Event Stat: ", event.str());
    break;
#endif
  case RdKafka::Event::EVENT_LOG:
    Util::iprint("Kafka Event Log-", event.severity(), ": ", event.fac(), ": ",
                 event.str());
    break;
  default:
    Util::dprint(F, "Kafka Event : ", RdKafka::err2str(event.err()), ": ",
                 event.str());
    break;
  }
}

KafkaProducer::KafkaProducer(shared_ptr<Config> _conf) : conf(move(_conf))
{
  if (conf->kafka_broker.empty() || conf->kafka_topic.empty()) {
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
  if (!conf->kafka_conf.empty()) {
    set_kafka_conf_file(conf->kafka_conf);
  }

  if (conf->mode_grow || conf->input_type == InputType::Nic) {
    period_chk = true;
    last_time = std::chrono::steady_clock::now();
  }

  set_kafka_threshold();
  show_kafka_conf();

  string errstr;

  // Create producer handle
  kafka_producer.reset(RdKafka::Producer::create(kafka_gconf.get(), errstr));
  if (!kafka_producer) {
    throw runtime_error("Failed to create kafka producer handle: " + errstr);
  }

  // Create topic handle
  kafka_topic.reset(RdKafka::Topic::create(
      kafka_producer.get(), conf->kafka_topic, kafka_tconf.get(), errstr));
  if (!kafka_topic) {
    throw runtime_error("Failed to create kafka topic handle: " + errstr);
  }
}

void KafkaProducer::set_kafka_conf()
{
  string errstr;

  // Set configuration properties
  if (kafka_gconf->set("metadata.broker.list", conf->kafka_broker, errstr) !=
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
    case KafkaConfType::Global:
      if (kafka_gconf->set(entry.key, entry.value, errstr) !=
          RdKafka::Conf::CONF_OK) {
        throw runtime_error("Failed to set default kafka global config: " +
                            string(entry.key) + ": " + errstr);
      }
      break;
    case KafkaConfType::Topic:
      if (kafka_tconf->set(entry.key, entry.value, errstr) !=
          RdKafka::Conf::CONF_OK) {
        throw runtime_error("Failed to set default kafka topic config: " +
                            string(entry.key) + ": " + errstr);
      }
      break;
    default:
      throw runtime_error("Failed to set default kafka config: unknown type");
      break;
    }
  }
}

void KafkaProducer::set_kafka_conf_file(const string& conf_file)
{
  constexpr char global_section[] = "[global]";
  constexpr char topic_section[] = "[topic]";
  RdKafka::Conf* kafka_conf_ptr = nullptr;
  string line, option, value;
  size_t line_num = 0, offset = 0;
  string errstr;
  ifstream conf_stream(conf_file);

  if (!conf_stream.is_open()) {
    throw runtime_error("Failed to open kafka config file: " + conf_file);
  }

  while (getline(conf_stream, line)) {
    line_num++;
    Util::trim(line);
    if (line.find('#') == 0) {
      continue;
    }
    if (line.find(global_section) != string::npos) {
      kafka_conf_ptr = kafka_gconf.get();
      continue;
    }
    if (line.find(topic_section) != string::npos) {
      kafka_conf_ptr = kafka_tconf.get();
      continue;
    }
    if (kafka_conf_ptr == nullptr) {
      continue;
    }

    offset = line.find_first_of(":=");
    option = line.substr(0, offset);
    value = line.substr(offset + 1, line.length());

    if (offset == string::npos) {
      continue;
    }

    if (kafka_conf_ptr->set(Util::trim(option), Util::trim(value), errstr) !=
        RdKafka::Conf::CONF_OK) {
      throw runtime_error(string("Failed to set kafka config: ")
                              .append(errstr)
                              .append(": ")
                              .append(line)
                              .append("(")
                              .append(to_string(line_num))
                              .append(" line)"));
    }
    Util::dprint(F, Util::trim(option), "=", Util::trim(value));
  }

  conf_stream.close();
}

void KafkaProducer::set_kafka_threshold()
{
  string queue_buffering_max_kbytes, queue_buffering_max_messages,
      message_max_bytes;

  kafka_gconf->get("queue.buffering.max.kbytes", queue_buffering_max_kbytes);
  kafka_gconf->get("queue.buffering.max.messages",
                   queue_buffering_max_messages);
  kafka_gconf->get("message.max.bytes", message_max_bytes);
#if 0
  // TODO: optimize redundancy_kbytes, redundancy_counts
  const size_t redundancy_kbytes = conf->queue_size;
  const size_t redundancy_counts = 1;

  if (conf->queue_size > stoul(message_max_bytes)) {
    throw runtime_error("Queue size too large. Increase message.max.bytes "
                        "value in the config file or lower queue value: " +
                        to_string(conf->queue_size));
  }
  queue_threshold =
      (stoul(queue_buffering_max_kbytes) * 1000 - redundancy_kbytes) /
      conf->queue_size;
  if (queue_threshold > stoul(queue_buffering_max_messages)) {
    queue_threshold = stoul(queue_buffering_max_messages) - redundancy_counts;
  }
  Util::dprint(F, "queue_threshold=", queue_threshold);
#endif
}

void KafkaProducer::show_kafka_conf() const
{
  // show kafka producer config
  for (int pass = 0; pass < 2; pass++) {
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

void KafkaProducer::wait_queue(const int count) noexcept
{
  if (kafka_producer->outq_len() <= count) {
    return;
  }
  Util::dprint(
      F, "waiting for delivery of messages: ", kafka_producer->outq_len());
  do {
    // FIXME: test effects of timeout (original 1000)
    kafka_producer->poll(100);
  } while (kafka_producer->outq_len() > count);
}

bool KafkaProducer::produce_core(const string& message) noexcept
{
  // Produce message
  RdKafka::ErrorCode resp = kafka_producer->produce(
      kafka_topic.get(), RdKafka::Topic::PARTITION_UA,
      RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      const_cast<char*>(message.c_str()), message.size(), nullptr,
      reinterpret_cast<void*>(&(this->pac)));

  kafka_producer->poll(0);
  switch (resp) {
  case RdKafka::ERR_NO_ERROR:
    pac.produce_cnt++;
    Util::dprint(F, "A message queue entry success(", queue_data_cnt,
                 "converted data, ", message.size(), "bytes, ", pac.ack_cnt,
                 "/", pac.produce_cnt, "acked)");
    break;

  default:
    Util::eprint("A message queue entry failed(", queue_data_cnt,
                 "converted data, ", message.size(), "bytes, ", pac.ack_cnt,
                 "/", pac.produce_cnt, "acked): ", RdKafka::err2str(resp));
    return false;
  }

  return true;
}

bool KafkaProducer::period_queue_flush() noexcept
{
  if (!period_chk) {
    return false;
  }

  current_time = std::chrono::steady_clock::now();
  time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(
      current_time - last_time);
  if (time_diff.count() > static_cast<double>(conf->queue_period)) {
    Util::dprint(
        F, "Time lapse since last message queue entry: ", time_diff.count());
    return true;
  }

  return false;
}

bool KafkaProducer::produce(const string& message, bool flush) noexcept
{

  queue_data += message;
  if (!message.empty() && !flush) {
    queue_data_cnt++;
  }

  if (flush || period_queue_flush() ||
      queue_data.length() >= conf->queue_size) {
    if (!produce_core(queue_data)) {
      queue_data.clear();
      queue_data_cnt = 0;
      // FIXME: error handling
      return false;
    }
    queue_data.clear();
    queue_data_cnt = 0;

    if (period_chk) {
      last_time = std::chrono::steady_clock::now();
    }
    if (flush) {
      wait_queue(0);
    } else {
      wait_queue(queue_threshold);
    }
  } else {
    queue_data += '\n';
  }

  return true;
}

size_t KafkaProducer::get_max_bytes() const noexcept
{
  string message_max_bytes;
  kafka_gconf->get("message.max.bytes", message_max_bytes);

  return stoull(message_max_bytes);
}

KafkaProducer::~KafkaProducer()
{
  if (queue_data.size()) {
    this->produce("", true);
  }

  if (!RdKafka::wait_destroyed(1000)) {
    Util::eprint("Failed to release kafka resources");
  }
}

/**
 * FileProducer
 */

FileProducer::FileProducer(shared_ptr<Config> _conf) : conf(move(_conf))
{
  if (!conf->output.empty()) {
    if (!open()) {
      throw runtime_error("Failed to open output file: " + conf->output);
    }
  }
}

FileProducer::~FileProducer()
{
  if (file.is_open()) {
    file.close();
  }
}

bool FileProducer::produce(const string& message, bool flush) noexcept
{
  file << message << "\n";
  file.flush();

  // FIXME: check error?

  return true;
}

bool FileProducer::open() noexcept
{
  file.open(conf->output, ios::out);
  if (!file.is_open()) {
    Util::eprint("Failed to open file: ", conf->output);
    return false;
  }

  return true;
}

size_t FileProducer::get_max_bytes() const noexcept
{
  return default_produce_max_bytes;
}

/**
 * NullProducer
 */

NullProducer::NullProducer(shared_ptr<Config> _conf) : conf(move(_conf)) {}

NullProducer::~NullProducer() = default;

bool NullProducer::produce(const string& message, bool flush) noexcept
{
  return true;
}

size_t NullProducer::get_max_bytes() const noexcept
{
  return default_produce_max_bytes;
}

// vim: et:ts=2:sw=2
