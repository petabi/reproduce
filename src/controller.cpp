#include <cstdarg>
#include <cstring>

#include "controller.h"
#include "report.h"

using namespace std;

static constexpr size_t MESSAGE_SIZE = 1024;

Controller::Controller(Config _conf) : conf(move(_conf))
{
  Util::set_debug(conf.mode_debug);

  if (!set_converter()) {
    throw runtime_error("Failed to set the converter");
  }
  if (!set_producer()) {
    throw runtime_error("Failed to set the producer");
  }
}

Controller::~Controller()
{
  close_pcap();
  close_log();
}

void Controller::run()
{
  Report report(conf);
  char imessage[MESSAGE_SIZE];
  char omessage[MESSAGE_SIZE];
  int length;
  ControllerResult ret;

  if (conf.count_skip) {
    if (!(this->*skip_data)(conf.count_skip)) {
      Util::dprint(F, "failed to skip(%d)", conf.count_skip);
    }
  }

  Util::dprint(F, "start");
  report.start();

  while (true) {
    length = MESSAGE_SIZE;
    ret = (this->*get_next_data)(imessage, length);
    if (ret == ControllerResult::SUCCESS) {
      length = conv->convert(imessage, length, omessage, MESSAGE_SIZE);
      if (length <= 0) {
        report.fail();
        continue;
      }
    } else if (ret == ControllerResult::FAIL) {
      // TODO: How to handle error
      break;
    } else if (ret == ControllerResult::NO_MORE) {
      // TODO: How to handle error
      break;
    } else {
      break;
    }

    prod->produce(omessage);
    Util::mprint(omessage);
    report.process(length);

    if (check_count(report.get_sent_count())) {
      break;
    }
  }

  report.end();
  Util::dprint(F, "end");
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

bool Controller::set_converter()
{
  switch (get_converter_type()) {
  case ConverterType::PCAP:
    uint32_t l2_type;
    l2_type = open_pcap(conf.input);
    conv = make_unique<PacketConverter>(l2_type);
    get_next_data = &Controller::get_next_pcap;
    skip_data = &Controller::skip_pcap;
    Util::dprint(F, "input type: PCAP");
    break;
  case ConverterType::LOG:
    open_log(conf.input);
    conv = make_unique<LogConverter>();
    get_next_data = &Controller::get_next_log;
    skip_data = &Controller::skip_log;
    Util::dprint(F, "input type: LOG");
    break;
  case ConverterType::NONE:
    conv = make_unique<NullConverter>();
    get_next_data = &Controller::get_next_null;
    skip_data = &Controller::skip_null;
    Util::dprint(F, "input type: NONE");
    break;
  default:
    return false;
  }

  return true;
}

bool Controller::set_producer()
{
  switch (get_producer_type()) {
  case ProducerType::KAFKA:
    prod = make_unique<KafkaProducer>(conf);
    Util::dprint(F, "output type: KAFKA");
    break;
  case ProducerType::FILE:
    prod = make_unique<FileProducer>(conf);
    Util::dprint(F, "output type: FILE");
    break;
  case ProducerType::NONE:
    prod = make_unique<NullProducer>(conf);
    Util::dprint(F, "output type: NONE");
    break;
  default:
    return false;
  }

  return true;
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

void Controller::open_log(const std::string& filename)
{
  logfile.open(filename.c_str(), fstream::in);
  if (!logfile.is_open()) {
    throw runtime_error("Failed to open input file: " + filename);
  }
}

void Controller::close_log()
{
  if (logfile.is_open()) {
    logfile.close();
  }
}

ControllerResult Controller::get_next_pcap(char* imessage, size_t imessage_len)
{
  size_t pp_len = sizeof(pcap_pkthdr);
  if (fread(imessage, 1, pp_len, pcapfile) != pp_len) {
    return ControllerResult::NO_MORE;
  }

#if 0
  // we assume packet length < buffer length
  if (packet_len >= imessage_len) {
    throw runtime_error("Packet buffer too small");
  }
#endif

  size_t read_len;
  auto* pp = reinterpret_cast<pcap_pkthdr*>(imessage);
  if (imessage_len < pp->caplen + pp_len) {
    read_len = imessage_len - pp_len - 1;
  } else {
    read_len = pp->caplen;
  }
  if (fread(reinterpret_cast<char*>(imessage + static_cast<int>(pp_len)), 1,
            read_len, pcapfile) != read_len) {
    return ControllerResult::FAIL;
  }
  imessage_len = read_len;

  return ControllerResult::SUCCESS;
}

ControllerResult Controller::get_next_log(char* imessage, size_t imessage_len)
{
  string line;
  if (!logfile.getline(imessage, imessage_len)) {
    if (logfile.eof()) {
      return ControllerResult::NO_MORE;
    } else if (logfile.bad() || logfile.fail()) {
      return ControllerResult::FAIL;
    }
  }
  imessage_len = strlen(imessage);

  return ControllerResult::SUCCESS;
}

ControllerResult Controller::get_next_null(char* imessage, size_t imessage_len)
{
  return ControllerResult::SUCCESS;
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

bool Controller::skip_log(size_t count_skip)
{
  char buf[1];
  size_t count = 0;
  while (count < count_skip) {
    if (!logfile.getline(buf, 1)) {
      if (logfile.eof()) {
        return false;
      } else if (logfile.bad() || logfile.fail()) {
        return false;
      }
    }
    count++;
  }

  return true;
}

bool Controller::skip_null(size_t count) { return true; }

bool Controller::check_count(const size_t sent_count) const noexcept
{
  if (conf.count_send == 0 || sent_count < conf.count_send) {
    return false;
  }

  return true;
}

// vim: et:ts=2:sw=2
