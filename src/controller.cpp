#include <cstdarg>
#include <cstring>

#include "controller.h"
#include "options.h"
#include "producer.h"

using namespace std;

static constexpr size_t MESSAGE_SIZE = 1024;
static constexpr char sample_data[] =
    "1531980827 Ethernet2 a4:7b:2c:1f:eb:61 40:61:86:82:e9:26 IP 4 5 0 10240 "
    "58477 64 127 47112 59.7.91.107 123.141.115.52 ip_opt TCP 62555 80 "
    "86734452 2522990538 20 A 16425 7168 0";

Controller::Controller(Config _conf) : conf(move(_conf)) {}

Controller::~Controller() {}

void Controller::run()
{
  Options opt(conf);

  opt.show_options();
  opt.dprint(F, "start");
  opt.start_evaluation();

  unique_ptr<Converter> cp = nullptr;
  unique_ptr<KafkaProducer> rpp = nullptr;
  char imessage[MESSAGE_SIZE];
  char omessage[MESSAGE_SIZE];
  int length = 0;

  if (conf.mode_parse) {
    copy(sample_data, sample_data + sizeof(sample_data), imessage);
    length = strlen(imessage);
    opt.dprint(F, "imessage=", imessage, " (", length, ")");
  } else {
    switch (opt.get_input_type()) {
    case InputType::PCAP:
      cp = make_unique<PacketConverter>();
      opt.dprint(F, "input type=Pcap");
      break;
    case InputType::LOG:
      cp = make_unique<LogConverter>();
      opt.dprint(F, "input type=Log");
      break;
    default:
      throw runtime_error("Specify the appropriate input (See help)");
    }
  }
#if 0
  // FIXME:
  if (conf.count_skip) {
    if (!cp->skip(conf.count_skip)) {
      opt.dprint(F, "failed to skip(%d)", conf.count_skip);
    }
  }
#endif
  if (!conf.mode_kafka) {
    rpp = make_unique<KafkaProducer>(opt);
  }

  while (true) {
    opt.process_evaluation(length);
    if (opt.check_count()) {
      break;
    }
    if (!conf.mode_parse) {
      length = cp->convert(imessage, MESSAGE_SIZE, omessage, MESSAGE_SIZE);
      if (length > 0) {
        // do nothing
      } else if (length == static_cast<int>(ConverterResult::FAIL)) {
        opt.increase_fail();
        continue;
      } else if (length == static_cast<int>(ConverterResult::NO_MORE)) {
        break;
      } else {
        // can't get here
      }
    }
    if (!conf.mode_kafka) {
      rpp->produce(omessage);
    }
    opt.mprint(omessage);
    opt.fprint(omessage);
  }
  opt.report_evaluation();
  opt.dprint(F, "end");
}

// vim: et:ts=2:sw=2
