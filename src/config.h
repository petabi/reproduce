#ifndef CONFIG_H
#define CONFIG_H

#include <string>

#include "util.h"

enum class InputType { Pcap, Pcapng, Nic, Log, Dir };

enum class OutputType {
  None,
  Kafka,
  File,
};

class Config {
public:
  // user
  bool mode_eval{false};        // report statistics
  bool mode_grow{false};        // convert while tracking the growing file
  bool mode_polling_dir{false}; // polling the input directory
  size_t count_skip{0};         // count to skip
  size_t queue_size{0};         // how many bytes send once
  size_t queue_period{0};       // how much time keep queued data
  std::string input;            // input: packet/log/none
  std::string output;           // output: kafka/file/none
  std::string offset_prefix;    // prefix of offset file to read and write
  std::string packet_filter;
  std::string kafka_broker;
  std::string kafka_topic;
  std::string kafka_conf;
  std::string pattern_file;
  std::string file_prefix; // prefix for file name to send when you want to send
                           // multiple files or directory

  uint8_t datasource_id = 1;
  uint32_t initial_seq_no = 0;

  float entropy_ratio = 0.9; // entropy break point to drop a seesion

  // internal
  size_t count_send{0};
  InputType input_type;
  OutputType output_type;

  Config() = default;
  Config(const Config&) = default;
  auto operator=(const Config&) -> Config& = default;
  Config(Config&&) = default;
  auto operator=(Config &&) -> Config& = delete;
  ~Config() = default;
  auto set(int argc, char** argv) -> bool;
  void show() const noexcept;

private:
  void help() const noexcept;
  void set_default() noexcept;
  void check() const;
};

#endif

// vim: et:ts=2:sw=2
