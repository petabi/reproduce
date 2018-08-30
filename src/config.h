#ifndef CONFIG_H
#define CONFIG_H

#include <string>

class Config {
public:
  bool mode_debug{false}; // print debug messages
  bool mode_eval{false};  // report statistics
  size_t count_send{0};   // count to send
  size_t count_skip{0};   // count to skip
  size_t queue_size{0};   // how many bytes send once
  std::string input;      // input: packet/log/none
  std::string output;     // output: kafka/file/none
  std::string filter;     // tcpdump filter string
  std::string broker;     // kafka broker
  std::string topic;      // kafka topic
  Config() = default;
  Config(const Config&) = default;
  Config& operator=(const Config&) = default;
  Config(Config&&) = default;
  Config& operator=(Config&&) = delete;
  ~Config() = default;
  bool get_argument(int argc, char** argv);

private:
  void help() const;
  void set_default();
};

#endif

// vim: et:ts=2:sw=2
