#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include "util.h"

enum class InputType { None, Pcap, Pcapng, Nic, Log, Dir };

enum class OutputType {
  None,
  Kafka,
  File,
};

class Config {
public:
  // user
  bool mode_eval{false};     // report statistics
  bool mode_grow{false};     // convert while tracking the growing file
  size_t count_skip{0};      // count to skip
  size_t queue_size{0};      // how many bytes send once
  size_t queue_period{0};    // how much time keep queued data
  std::string input;         // input: packet/log/none
  std::string output;        // output: kafka/file/none
  std::string offset_prefix; // prefix of offset file to read and write
  std::string packet_filter;
  std::string kafka_broker;
  std::string kafka_topic;
  std::string kafka_conf;
  std::string pattern_file;

  // internal
  size_t count_send{0};
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
