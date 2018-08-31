#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "converter.h"
#include "producer.h"
#include "util.h"

class Controller {
public:
  Controller() = delete;
  Controller(const Config&);
  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;
  Controller(Controller&&) = delete;
  Controller& operator=(const Controller&&) = delete;
  ~Controller();
  void run();

private:
  Config conf;
  ConverterType get_converter_type() const;
  ProducerType get_producer_type() const;
  uint32_t l2_type;
  FILE* pcapfile;
  uint32_t open_pcap(const std::string& filename);
  void close_pcap();
  bool skip_pcap(size_t count_skip);
  int get_next_pcap_format(char* imessage, size_t imessage_len);
  bool check_count(const size_t sent_count) const noexcept;
};

#endif

// vim: et:ts=2:sw=2
