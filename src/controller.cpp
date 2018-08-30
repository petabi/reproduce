#include <cstdarg>
#include <cstring>

#include "controller.h"

using namespace std;

static constexpr size_t MESSAGE_SIZE = 1024;

Controller::Controller(Config _conf) : conf(move(_conf)) {}

/* TODO: fix for controller
PacketConverter::PacketConverter(PacketConverter&& other) noexcept
{
  FILE* tmp = other.pcapfile;
  other.pcapfile = nullptr;
  if (pcapfile != nullptr) {
    fclose(pcapfile);
  }
  pcapfile = tmp;
}
*/

Controller::~Controller() { close_pcap(); }

void Controller::run()
{
  Options opt(conf);

  opt.dprint(F, "start");
  opt.show_options();

  unique_ptr<Converter> conv = nullptr;
  switch (get_converter_type()) {
  case ConverterType::PCAP:
    l2_type = open_pcap(conf.input);
    conv = make_unique<PacketConverter>(l2_type);
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
    if (get_next_pcap_format(imessage, MESSAGE_SIZE)) {
      length = conv->convert(imessage, MESSAGE_SIZE, omessage, MESSAGE_SIZE);
    }
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

uint32_t Controller::open_pcap(const string& filename)
{
  pcapfile = fopen(filename.c_str(), "r");
  if (pcapfile == nullptr) {
    throw runtime_error("Failed to open input file: " + filename);
  }

  struct pcap_file_header pfh;
  size_t pfh_len = sizeof(pfh);
  if (fread(&pfh, 1, pfh_len, pcapfile) != pfh_len) {
    throw runtime_error("Invalid input pcap file: " + filename);
  }
  if (pfh.magic != 0xa1b2c3d4) {
    throw runtime_error("Invalid input pcap file: " + filename);
  }

  return pfh.linktype;
}

void Controller::close_pcap()
{
  if (pcapfile != nullptr) {
    fclose(pcapfile);
  }
}

bool Controller::skip_pcap(size_t count_skip)
{
  struct pcap_pkthdr pp;
  size_t count = 0;
  size_t pp_len = sizeof(pp);

  while (count < count_skip) {
    if (fread(&pp, 1, pp_len, pcapfile) != pp_len) {
      return false;
    }
    fseek(pcapfile, pp.caplen, SEEK_CUR);
    count++;
  }

  return true;
}

int Controller::get_next_pcap_format(char* imessage, size_t imessage_len)
{
  size_t pp_len = sizeof(pcap_pkthdr);
  if (fread(imessage, 1, pp_len, pcapfile) != pp_len) {
    return false;
  }
  auto* pp = reinterpret_cast<pcap_pkthdr*>(imessage);

#if 0
  // we assume packet_len < PACKET_BUF_SIZE
  if (packet_len >= PACKET_BUF_SIZE) {
    throw runtime_error("Packet buffer too small");
  }
#endif

  size_t read_len;
  if (imessage_len < pp->caplen + pp_len) {
    read_len = imessage_len - pp->caplen - 1;
  } else {
    read_len = pp->caplen;
  }
  if (fread(imessage + (int)pp_len, 1, read_len, pcapfile) != read_len) {
    return false;
  }

  return true;
}

// vim: et:ts=2:sw=2
