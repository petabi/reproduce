#include <cstdarg>
#include <cstring>

#include "controller.h"
#include "options.h"
#include "producer.h"

using namespace std;

Controller::Controller(Config _conf) : conf(move(_conf)) {}

void Controller::run()
{
#if 0
  // test
  unique_ptr<Converter> conv = nullptr;
  unique_ptr<Producer> prod = nullptr; 
  Options opt(conf);
  conv = make_unique<PacketConverter>(conf.input);
  prod = make_unique<KafkaProducer>(opt);
#endif

  return;
}

// vim: et:ts=2:sw=2
