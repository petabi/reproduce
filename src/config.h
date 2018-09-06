#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include "util.h"

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
  bool mode_debug{false}; // print debug messages
  bool mode_eval{false};  // report statistics
  size_t count_send{0};   // count to send
  size_t count_skip{0};   // count to skip
  size_t queue_size{0};   // how many bytes send once
  std::string input;      // input: packet/log/none
  InputType input_type;
  std::string output; // output: kafka/file/none
  OutputType output_type;
  std::string filter; // tcpdump filter string
  std::string broker; // kafka broker
  std::string topic;  // kafka topic
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
