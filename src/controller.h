#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "converter.h"
#include "options.h"
#include "producer.h"

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
  ConverterType get_converter_type() const;
  ProducerType get_producer_type() const;
};

#endif

// vim: et:ts=2:sw=2
