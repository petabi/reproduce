#include <fstream>

#include "config.h"
#include "producer.h"
#include "util.h"

using namespace std;

extern "C" {

void kafka_producer_free(InnerProducer* ptr);
auto kafka_producer_new(const char* broker, const char* topic)
    -> InnerProducer*;
auto kafka_producer_send(InnerProducer* ptr, const char* msg, size_t len)
    -> size_t;
}

/**
 * KafkaProducer
 */

Producer::~Producer() = default;

KafkaProducer::KafkaProducer(const Config* conf)
    : queue_period(config_queue_period(conf)),
      queue_size(config_queue_size(conf))
{
  if (!strlen(config_kafka_broker(conf)) || !strlen(config_kafka_topic(conf))) {
    throw runtime_error("Invalid constructor parameter");
  }

  if (config_mode_grow(conf) || config_input_type(conf) == InputType::Nic) {
    period_chk = true;
    last_time = std::chrono::steady_clock::now();
  }

  inner =
      kafka_producer_new(config_kafka_broker(conf), config_kafka_topic(conf));
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
  if (time_diff.count() > static_cast<double>(queue_period)) {
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

  if (flush || period_queue_flush() || queue_data.length() >= queue_size) {
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

FileProducer::FileProducer(const Config* conf)
{
  auto filename = config_output(conf);
  if (strlen(filename)) {
    file.open(config_output(conf), ios::out);
    if (!file.is_open()) {
      Util::eprint("Failed to open file: ", config_output(conf));
      throw runtime_error(string("Failed to open output file: ") +
                          config_output(conf));
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

auto FileProducer::get_max_bytes() const noexcept -> size_t
{
  return default_produce_max_bytes;
}

/**
 * NullProducer
 */

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
