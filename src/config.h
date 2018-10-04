#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include "util.h"

constexpr size_t queue_size_min = 100;
constexpr size_t queue_size_max = 900000;

enum class InputType {
  NONE,
  PCAP,
  PCAPNG,
  NIC,
  LOG,
};

enum class OutputType {
  NONE,
  KAFKA,
  FILE,
};

class Config {
public:
  // user
  bool mode_debug{false}; // print debug messages
  bool mode_eval{false};  // report statistics
  bool mode_grow{false};  // convert while tracking the growing file
  size_t count_skip{0};   // count to skip
  size_t queue_size{0};   // how many bytes send once
  std::string input;      // input: packet/log/none
  std::string output;     // output: kafka/file/none
  std::string packet_filter;
  std::string kafka_broker;
  std::string kafka_topic;
  std::string kafka_conf;

  // internal
  size_t count_send{0};
  bool mode_auto_queue{false};
  size_t calculate_interval{0};
  InputType input_type;
  OutputType output_type;

  Config() = default;
  Config(const Config&) = default;
  Config& operator=(const Config&) = default;
  Config(Config&&) = default;
  Config& operator=(Config&&) = delete;
  ~Config() = default;
  bool set(int argc, char** argv);
  void show() const noexcept;

private:
  void help() const noexcept;
  void set_default() noexcept;
  void check() const;
};

#endif

// vim: et:ts=2:sw=2
