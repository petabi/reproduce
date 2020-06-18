#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <dirent.h>

#include <pcap/pcap.h>

#include "converter.h"
#include "producer.h"
#include "report.h"
#include "util.h"

namespace GetData {
enum class Status { Fail = -2, No_more = -1, Success = 0 };
}

class Controller {
public:
  Controller() = delete;
  Controller(Config const&);
  Controller(const Controller&) = delete;
  auto operator=(const Controller&) -> Controller& = delete;
  Controller(Controller&&) = delete;
  auto operator=(const Controller &&) -> Controller& = delete;
  ~Controller();
  void run();

private:
  time_t launch_time{0};
  std::shared_ptr<Config> conf;
  std::unique_ptr<Converter> conv;
  std::unique_ptr<Producer> prod;
  uint32_t seq_no = 1; /* use lower 24-bit */
  static pcap_t* pcd;
  FILE* pcapfile{nullptr};
  std::ifstream logfile;
  auto (Controller::*get_next_data)(char* imessage, size_t& imessage_len)
      -> GetData::Status;
  auto (Controller::*skip_data)(const size_t count_skip) -> bool;
  void run_split();
  void run_single();
  auto get_input_type() const -> InputType;
  auto get_output_type() const -> OutputType;
  auto set_converter() -> bool;
  auto set_producer() -> bool;
  auto open_nic(const std::string& devname) -> uint32_t;
  void close_nic();
  auto open_pcap(const std::string& filename) -> uint32_t;
  void close_pcap();
  void open_log(const std::string& filename);
  void close_log();
  auto read_offset(const std::string& filename) const -> uint32_t;
  void write_offset(const std::string& filename, uint32_t offset) const;
  auto get_next_nic(char* imessage, size_t& imessage_len) -> GetData::Status;
  auto get_next_pcap(char* imessage, size_t& imessage_len) -> GetData::Status;
  auto get_next_log(char* imessage, size_t& imessage_len) -> GetData::Status;
  auto get_next_file(DIR* dir) const -> std::string;
  auto traverse_directory(std::string& _path, const std::string& _prefix,
                          std::vector<std::string>& read_files)
      -> std::vector<std::string>;
  auto skip_pcap(const size_t count_skip) -> bool;
  auto skip_log(const size_t count_skip) -> bool;
  auto skip_null(const size_t count_skip) -> bool;
  auto check_count(const size_t sent_count) const noexcept -> bool;
  /* event_id(64bit) = upper 32-bit (current time in seconds)
                     + lower 24-bit (sequence number)
                     + lowest 8-bit (data source id) */
  auto event_id() -> uint64_t
  {
    return ((time(nullptr) << 32) | ((seq_no & 0x00FFFFFF) << 8) |
            conf->datasource_id);
  }
  auto get_seq_no(const int no) -> uint32_t
  {
    return ((seq_no + no) & 0x00FFFFFF);
  }
  static void signal_handler(int signal);
};

#endif

// vim: et:ts=2:sw=2
