#include <array>

#include "producer.h"

using namespace std;

extern "C" {

void kafka_producer_free(InnerProducer* ptr);
auto kafka_producer_new(const char* broker, const char* topic,
                        uint32_t idle_timeout_ms, uint32_t ack_timeout_ms)
    -> InnerProducer*;
auto kafka_producer_send(InnerProducer* ptr, const char* msg, size_t len)
    -> size_t;
}

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
    {KafkaConfType::Topic, "message.timeout.ms", "5000", "5000", "900000",
     "300000",
     "limit the time a produced message waits for successful delivery"},
}};

Producer::~Producer() = default;

KafkaProducer::KafkaProducer(shared_ptr<Config> _conf) : conf(move(_conf))
{
  if (conf->kafka_broker.empty() || conf->kafka_topic.empty()) {
    throw runtime_error("Invalid constructor parameter");
  }

  set_kafka_conf();
  if (!conf->kafka_conf.empty()) {
    set_kafka_conf_file(conf->kafka_conf);
  }

  if (conf->mode_grow || conf->input_type == InputType::Nic) {
    period_chk = true;
    last_time = std::chrono::steady_clock::now();
  }

  show_kafka_conf();

  uint32_t idle_timeout_ms = 540000;
  auto it = kafka_gconf.find("socket.timeout.ms");
  if (it != kafka_gconf.end()) {
    idle_timeout_ms = std::stoi(it->second);
  }
  uint32_t ack_timeout_ms = 5000;
  it = kafka_tconf.find("message.timeout.ms");
  if (it != kafka_tconf.end()) {
    ack_timeout_ms = std::stoi(it->second);
  }
  inner =
      kafka_producer_new(conf->kafka_broker.c_str(), conf->kafka_topic.c_str(),
                         idle_timeout_ms, ack_timeout_ms);
}

void KafkaProducer::set_kafka_conf()
{
  string errstr;

  // Set configuration properties: optional features
  for (const auto& entry : kafka_conf) {
    Util::dprint(F, entry.key, "=", entry.value, " (", entry.min, "~",
                 entry.max, ", ", entry.base, ")");
    switch (entry.type) {
    case KafkaConfType::Global:
      kafka_gconf[entry.key] = entry.value;
      break;
    case KafkaConfType::Topic:
      kafka_tconf[entry.key] = entry.value;
      break;
    default:
      throw runtime_error("Failed to set default kafka config: unknown type");
      break;
    }
  }
}

void KafkaProducer::set_kafka_conf_file(const string& conf_file)
{
#define GLOBALSECTION "[global]"
#define TOPICSECTION "[topic]"
  std::map<std::string, std::string>* kafka_conf_ptr = nullptr;
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
    if (line.find(GLOBALSECTION) != string::npos) {
      kafka_conf_ptr = &kafka_gconf;
      continue;
    }
    if (line.find(TOPICSECTION) != string::npos) {
      kafka_conf_ptr = &kafka_tconf;
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

    (*kafka_conf_ptr)[Util::trim(option)] = Util::trim(value);
    Util::dprint(F, Util::trim(option), "=", Util::trim(value));
  }

  conf_stream.close();
}

void KafkaProducer::show_kafka_conf() const
{
  // show kafka producer config
  for (int pass = 0; pass < 2; pass++) {
    const std::map<std::string, std::string>* dump;
    if (pass == 0) {
      dump = &kafka_gconf;
      Util::dprint(F, "### Global Config");
    } else {
      dump = &kafka_tconf;
      Util::dprint(F, "### Topic Config");
    }
    for (auto it : *dump) {
      Util::dprint(F, it.first, "=", it.second);
    }
  }
}

auto KafkaProducer::produce_core(const string& message) noexcept -> bool
{
  return kafka_producer_send(inner, message.data(), message.size()) > 0;
}

auto KafkaProducer::period_queue_flush() noexcept -> bool
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

auto KafkaProducer::produce(const char* message, size_t len,
                            bool flush) noexcept -> bool
{

  queue_data.append(message, len);
  if (len > 0 && !flush) {
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
  } else {
    queue_data += '\n';
  }

  return true;
}

auto KafkaProducer::get_max_bytes() const noexcept -> size_t
{
  return default_produce_max_bytes;
}

KafkaProducer::~KafkaProducer()
{
  if (queue_data.size()) {
    this->produce("", true);
  }

  kafka_producer_free(inner);
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

auto FileProducer::produce(const char* message, size_t len, bool flush) noexcept
    -> bool
{
  file.write(message, len);
  file << '\n';
  file.flush();

  // FIXME: check error?

  return true;
}

auto FileProducer::open() noexcept -> bool
{
  file.open(conf->output, ios::out);
  if (!file.is_open()) {
    Util::eprint("Failed to open file: ", conf->output);
    return false;
  }

  return true;
}

auto FileProducer::get_max_bytes() const noexcept -> size_t
{
  return default_produce_max_bytes;
}

/**
 * NullProducer
 */

NullProducer::NullProducer(shared_ptr<Config> _conf) : conf(move(_conf)) {}

NullProducer::~NullProducer() = default;

auto NullProducer::produce(const char* message, size_t len, bool flush) noexcept
    -> bool
{
  return true;
}

auto NullProducer::get_max_bytes() const noexcept -> size_t
{
  return default_produce_max_bytes;
}

// vim: et:ts=2:sw=2
