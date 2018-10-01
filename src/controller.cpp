#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <thread>

#include "controller.h"

using namespace std;

static constexpr size_t MAX_PACKET_LENGTH = 65535;
static constexpr size_t MESSAGE_SIZE =
    MAX_PACKET_LENGTH + sizeof(pcap_pkthdr) + 1;
volatile sig_atomic_t stop = 0;

Controller::Controller(Config _conf) : conf(move(_conf))
{
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
  char imessage[MESSAGE_SIZE];
  char omessage[MESSAGE_SIZE];
  size_t imessage_len = 0, omessage_len = 0;
  ControllerResult ret;
  size_t sent_count;
  Report report(&conf);

  if (signal(SIGINT, signal_handler) == SIG_ERR ||
      signal(SIGTERM, signal_handler) == SIG_ERR) {
    Util::eprint(F, "Failed to register signal");
    return;
  }

  if (conf.count_skip) {
    if (!invoke(skip_data, this, conf.count_skip)) {
      Util::dprint(F, "failed to skip(", conf.count_skip, ")");
    }
  }

  Util::dprint(F, "start");
  report.start();

  while (!stop) {
    imessage_len = MESSAGE_SIZE;
    ret = invoke(get_next_data, this, imessage, imessage_len);
    if (ret == ControllerResult::SUCCESS) {
      omessage_len =
          conv->convert(imessage, imessage_len, omessage, MESSAGE_SIZE);
    } else if (ret == ControllerResult::FAIL) {
      Util::eprint(F, "Failed to convert input data");
      break;
    } else if (ret == ControllerResult::NO_MORE) {
      if (conf.mode_grow) {
        // TODO: optimize time interval
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }
      break;
    } else {
      break;
    }

    if (omessage_len <= 0) {
      report.fail();
      continue;
    }

    prod->produce(omessage);
    report.process(imessage_len, omessage_len);

    sent_count = report.get_sent_count();
    Util::mprint(omessage, sent_count);

    if (check_count(sent_count)) {
      break;
    }
  }

  report.end();
  Util::dprint(F, "end");
}

InputType Controller::get_input_type() const
{
  const unsigned char mn_pcap_little_micro[4] = {0xd4, 0xc3, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_micro[4] = {0xa1, 0xb2, 0xc3, 0xd4};
  const unsigned char mn_pcap_little_nano[4] = {0x4d, 0x3c, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_nano[4] = {0xa1, 0xb2, 0x3c, 0x4d};
  const unsigned char mn_pcapng_little[4] = {0x4D, 0x3C, 0x2B, 0x1A};
  const unsigned char mn_pcapng_big[4] = {0x1A, 0x2B, 0x3C, 0x4D};

  // TODO: Check whether input is a network interface
  // return NIC;

  if (conf.input.empty()) {
    return InputType::NONE;
  }

  ifstream ifs(conf.input, ios::binary);
  if (!ifs.is_open()) {
    throw runtime_error("Failed to open input file: " + conf.input);
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
  }

  return InputType::LOG;
}

OutputType Controller::get_output_type() const
{
  if (conf.output.empty()) {
    return OutputType::KAFKA;
  }

  if (conf.output == "none") {
    return OutputType::NONE;
  }

  return OutputType::FILE;
}

bool Controller::set_converter()
{
  conf.input_type = get_input_type();
  switch (conf.input_type) {
  case InputType::PCAP:
    uint32_t l2_type;
    l2_type = open_pcap(conf.input);
    conv = make_unique<PacketConverter>(l2_type);
    get_next_data = &Controller::get_next_pcap;
    skip_data = &Controller::skip_pcap;
    Util::dprint(F, "input type: PCAP");
    break;
  case InputType::LOG:
    open_log(conf.input);
    conv = make_unique<LogConverter>();
    get_next_data = &Controller::get_next_log;
    skip_data = &Controller::skip_log;
    Util::dprint(F, "input type: LOG");
    break;
  case InputType::NONE:
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
  conf.output_type = get_output_type();
  switch (conf.output_type) {
  case OutputType::KAFKA:
    prod = make_unique<KafkaProducer>(&conf);
    Util::dprint(F, "output type: KAFKA");
    break;
  case OutputType::FILE:
    prod = make_unique<FileProducer>(&conf);
    Util::dprint(F, "output type: FILE");
    break;
  case OutputType::NONE:
    prod = make_unique<NullProducer>(&conf);
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

ControllerResult Controller::get_next_pcap(char* imessage, size_t& imessage_len)
{
  size_t offset = 0;
  size_t pp_len = sizeof(pcap_pkthdr);

  offset = fread(imessage, 1, pp_len, pcapfile);
  if (offset != pp_len) {
    fseek(pcapfile, -offset, SEEK_CUR);
    return ControllerResult::NO_MORE;
  }

  auto* pp = reinterpret_cast<pcap_pkthdr*>(imessage);
  if (pp->caplen > MAX_PACKET_LENGTH) {
    Util::dprint(F,
                 "The captured packet size is abnormally large: ", pp->caplen);
    return ControllerResult::FAIL;
  }

  offset = fread(reinterpret_cast<char*>(imessage + static_cast<int>(pp_len)),
                 1, pp->caplen, pcapfile);
  if (offset != pp->caplen) {
    fseek(pcapfile, -(offset + pp_len), SEEK_CUR);
    return ControllerResult::NO_MORE;
  }
  imessage_len = pp->caplen;

  return ControllerResult::SUCCESS;
}

ControllerResult Controller::get_next_log(char* imessage, size_t& imessage_len)
{
  static size_t count = 0;
  static bool trunc = false;
  string line;

  count++;
  if (!logfile.getline(imessage, imessage_len)) {
    if (logfile.eof()) {
      logfile.clear();
      return ControllerResult::NO_MORE;
    }
    if (logfile.bad()) {
      return ControllerResult::FAIL;
    }
    if (logfile.fail()) {
      Util::eprint(F, "The message is truncated [", count, " line(",
                   imessage_len, " bytes)]");
      trunc = true;
      logfile.clear();
    }
  } else {
    imessage_len = strlen(imessage);
    if (trunc == true) {
      Util::eprint(F, "The message is truncated [", count, " line(",
                   imessage_len, " bytes)]");
      trunc = false;
    }
  }

  return ControllerResult::SUCCESS;
}

ControllerResult Controller::get_next_null(char* imessage, size_t& imessage_len)
{
  static constexpr char sample_data[] =
      "1531980827 Ethernet2 a4:7b:2c:1f:eb:61 40:61:86:82:e9:26 IP 4 5 0 10240 "
      "58477 64 127 47112 59.7.91.107 123.141.115.52 ip_opt TCP 62555 80 "
      "86734452 2522990538 20 A 16425 7168 0";

  memcpy(imessage, sample_data, strlen(sample_data) + 1);
  imessage_len = strlen(imessage);

  return ControllerResult::SUCCESS;
}

bool Controller::skip_pcap(const size_t count_skip)
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

bool Controller::skip_log(const size_t count_skip)
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

bool Controller::skip_null(const size_t count_skip) { return true; }

bool Controller::check_count(const size_t sent_count) const noexcept
{
  if (conf.count_send == 0 || sent_count < conf.count_send) {
    return false;
  }

  return true;
}

void Controller::signal_handler(int signal) { stop = 1; }

// vim: et:ts=2:sw=2
