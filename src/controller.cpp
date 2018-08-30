#include <cstdarg>
#include <cstring>

#include "controller.h"

using namespace std;

static constexpr size_t MESSAGE_SIZE = 1024;

Controller::Controller(Config _conf) : conf(move(_conf)) {}

void Controller::run()
{
  Options opt(conf);

  opt.dprint(F, "start");
  opt.show_options();

  unique_ptr<Converter> conv = nullptr;
  switch (get_converter_type()) {
  case ConverterType::PCAP:
    conv = make_unique<PacketConverter>();
    opt.dprint(F, "input type: PCAP");
    break;
  case ConverterType::LOG:
    conv = make_unique<LogConverter>();
    opt.dprint(F, "input type: LOG");
    break;
  default:
  case ConverterType::NONE:
    conv = make_unique<NullConverter>();
    opt.dprint(F, "input type: NONE");
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
  switch (get_producer_type()) {
  default:
  case ProducerType::KAFKA:
    prod = make_unique<KafkaProducer>();
    break;
  case ProducerType::FILE:
    prod = make_unique<FileProducer>();
    break;
  case ProducerType::NONE:
    prod = make_unique<NullProducer>();
    break;
  }

  opt.start_evaluation();

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
    prod->produce(omessage);
    opt.mprint(omessage);
    opt.fprint(omessage);
  }

  opt.report_evaluation();
  opt.dprint(F, "end");
}

ConverterType Controller::get_converter_type() const
{
  const unsigned char mn_pcap_little_micro[4] = {0xd4, 0xc3, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_micro[4] = {0xa1, 0xb2, 0xc3, 0xd4};
  const unsigned char mn_pcap_little_nano[4] = {0x4d, 0x3c, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_nano[4] = {0xa1, 0xb2, 0x3c, 0x4d};
  const unsigned char mn_pcapng_little[4] = {0x4D, 0x3C, 0x2B, 0x1A};
  const unsigned char mn_pcapng_big[4] = {0x1A, 0x2B, 0x3C, 0x4D};

  // TODO: Check whether input is a network interface
  // return NIC;

  ifstream ifs(conf.input, ios::binary);
  if (!ifs.is_open()) {
    return ConverterType::NONE;
  }

  ifs.seekg(0, ios::beg);
  unsigned char magic[4] = {0};
  ifs.read((char*)magic, sizeof(magic));

  if (memcmp(magic, mn_pcap_little_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_little_nano, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_nano, sizeof(magic)) == 0) {
    return ConverterType::PCAP;
  } else if (memcmp(magic, mn_pcapng_little, sizeof(magic)) == 0 ||
             memcmp(magic, mn_pcapng_big, sizeof(magic)) == 0) {
    return ConverterType::PCAPNG;
  } else {
    return ConverterType::LOG;
  }

  return ConverterType::NONE;
}

ProducerType Controller::get_producer_type() const
{
  if (conf.output.empty()) {
    return ProducerType::KAFKA;
  }

  if (conf.output == "none") {
    return ProducerType::NONE;
  }

  return ProducerType::FILE;
}

// vim: et:ts=2:sw=2
