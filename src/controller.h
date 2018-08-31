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
  FileRead (Controller::*get_next_data)(char* imessage, size_t imessage_len);
  bool (Controller::*skip_data)(size_t count);
  FILE* pcapfile;
  std::ifstream logfile;
  uint32_t open_pcap(const std::string& filename);
  void close_pcap();
  void open_log(const std::string& filename);
  void close_log();
  FileRead get_next_pcap(char* imessage, size_t imessage_len);
  FileRead get_next_log(char* imessage, size_t imessage_len);
  FileRead get_next_null(char* imessage, size_t imessage_len);
  bool skip_pcap(size_t count_skip);
  bool skip_log(size_t count);
  bool skip_null(size_t count);
  bool check_count(const size_t sent_count) const noexcept;
};

#endif

// vim: et:ts=2:sw=2
