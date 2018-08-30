#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "converter.h"
#include "options.h"
#include "producer.h"

enum class InputType {
  NONE,   // no type
  PCAP,   // pcap file
  PCAPNG, // pcapng file
  NIC,    // network interface
  LOG,    // log file
};

enum class OutputType {
  NONE,
  KAFKA,
  FILE,
};

class Controller {
public:
  Controller() = delete;
  Controller(Config);
  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;
  Controller(Controller&&) = delete;
  Controller& operator=(const Controller&&) = delete;
  ~Controller() = default;
  void run();

private:
  Config conf;
  InputType get_input_type() const;
  OutputType get_output_type() const;
};

#endif

// vim: et:ts=2:sw=2
