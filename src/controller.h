#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "converter.h"
#include "producer.h"
#include "util.h"

enum class FileRead { SUCCESS = 0, FAIL = -2, NO_MORE = -1 };

class Controller {
public:
  Controller() = delete;
  Controller(const Config&);
  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;
  Controller(Controller&&) noexcept;
  Controller& operator=(const Controller&&) = delete;
  ~Controller();
  void run();

private:
  Config conf;
  ConverterType get_converter_type() const;
  ProducerType get_producer_type() const;
  FileRead (Controller::*get_next_format)(char* imessage, size_t imessage_len);
  uint32_t l2_type;
  FILE* pcapfile;
  uint32_t open_pcap(const std::string& filename);
  void close_pcap();
  bool skip_pcap(size_t count_skip);
  bool check_count(const size_t sent_count) const noexcept;
  FileRead get_next_pcap_format(char* imessage, size_t imessage_len);
  FileRead get_next_log_format(char* imessage, size_t imessage_len);
  FileRead get_next_null_format(char* imessage, size_t imessage_len);
  std::ifstream logfile;
  void open_log(const std::string& filename);
  void close_log();
  bool skip_log(size_t count_skip);
};

#endif

// vim: et:ts=2:sw=2
