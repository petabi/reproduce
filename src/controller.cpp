#include <cstdarg>
#include <cstring>

#include "controller.h"
#include "options.h"
#include "producer.h"

using namespace std;

static constexpr size_t MESSAGE_SIZE = 1024;

Controller::Controller(Config _conf) : conf(move(_conf)) {}

void Controller::run()
{
  Options opt(conf);

  opt.show_options();
  opt.dprint(F, "start");
  opt.start_evaluation();

  unique_ptr<Converter> conv = nullptr;
  switch (get_input_type()) {
  case InputType::PCAP:
    conv = make_unique<PacketConverter>();
    opt.dprint(F, "input type: PCAP");
    break;
  case InputType::LOG:
    conv = make_unique<LogConverter>();
    opt.dprint(F, "input type: LOG");
    break;
  case InputType::NONE:
    conv = make_unique<NullConverter>();
    opt.dprint(F, "input type: NONE");
  default:
    throw runtime_error("Specify the appropriate input (See help)");
  }

#if 0
  // FIXME:
  if (conf.count_skip) {
    if (!conv->skip(conf.count_skip)) {
      opt.dprint(F, "failed to skip(%d)", conf.count_skip);
    }
  }
#endif

  unique_ptr<Producer> prod = nullptr;
  switch (get_output_type()) {
  case OutputType::KAFKA:
    prod = make_unique<KafkaProducer>();
    break;
  case OutputType::FILE:
    prod = make_unique<FileProducer>();
    break;
  case OutputType::NONE:
    prod = make_unique<NullProducer>();
    break;
  }

  char imessage[MESSAGE_SIZE];
  char omessage[MESSAGE_SIZE];
  int length = 0;
  while (true) {
    opt.process_evaluation(length);
    if (opt.check_count()) {
      break;
    }
    length = conv->convert(imessage, MESSAGE_SIZE, omessage, MESSAGE_SIZE);
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
    if (!conf.mode_kafka) {
      prod->produce(omessage);
    }
    opt.mprint(omessage);
    opt.fprint(omessage);
  }
  opt.report_evaluation();
  opt.dprint(F, "end");
}

InputType Controller::get_input_type() const
{
  const unsigned char mn_pcap_little_micro[4] = {0xd4, 0xc3, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_micro[4] = {0xa1, 0xb2, 0xc3, 0xd4};
  const unsigned char mn_pcap_little_nano[4] = {0x4d, 0x3c, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_nano[4] = {0xa1, 0xb2, 0x3c, 0x4d};
  const unsigned char mn_pcapng_little[4] = {0x4D, 0x3C, 0x2B, 0x1A};
  const unsigned char mn_pcapng_big[4] = {0x1A, 0x2B, 0x3C, 0x4D};

  // TODO:Check whether input is a network interface
  // return NIC;

  ifstream ifs(conf.input, ios::binary);
  if (!ifs.is_open()) {
    return InputType::NONE;
  }

  ifs.seekg(0, ios::beg);
  unsigned char magic[4] = {0};
  ifs.read((char*)magic, sizeof(magic));

  if (memcmp(magic, mn_pcap_little_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_little_nano, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_nano, sizeof(magic)) == 0) {
    return InputType::PCAP;
  } else if (memcmp(magic, mn_pcapng_little, sizeof(magic)) == 0 ||
             memcmp(magic, mn_pcapng_big, sizeof(magic)) == 0) {
    return InputType::PCAPNG;
  } else {
    return InputType::LOG;
  }

  return InputType::NONE;
}

OutputType Controller::get_output_type() const { return OutputType::KAFKA; }

// vim: et:ts=2:sw=2
