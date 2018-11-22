#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <thread>

#include <pcap/pcap.h>

#include "controller.h"

using namespace std;

static constexpr size_t max_packet_length = 65535;
static constexpr size_t message_size = 102400;
volatile sig_atomic_t stop = 0;
pcap_t* Controller::pcd = nullptr;

Controller::Controller(Config _conf)
{
  conf = make_shared<Config>(_conf);

  if (!set_converter()) {
    throw runtime_error("failed to set the converter");
  }
  if (!set_producer()) {
    throw runtime_error("failed to set the producer");
  }
}

Controller::~Controller()
{
  close_nic();
  close_pcap();
  close_log();
}

void Controller::run()
{
  if (conf->input_type == InputType::Dir) {
    run_split();
  } else {
    run_single();
  }
}

void Controller::run_split()
{
  string filename;
  string dir_path = conf->input;
  if (dir_path[dir_path.length() - 1] != '/') {
    dir_path += '/';
  }
  DIR* dir = opendir(dir_path.c_str());
  if (dir == nullptr) {
    throw runtime_error("failed to open the directory: " + dir_path);
  }
  while (!stop) {
    filename = get_next_file(dir);
    if (filename == "." || filename == "..") {
      continue;
    } else if (filename.empty()) {
      break;
    }
    conf->input = dir_path + filename;
    if (!set_converter()) {
      closedir(dir);
      throw runtime_error("failed to set the converter");
    }
    this->run();
    close_pcap();
    close_log();
  }
  closedir(dir);
}

void Controller::run_single()
{
  char imessage[message_size];
  char omessage[message_size];
  size_t imessage_len = 0, omessage_len = 0;
  ControllerResult ret;
  size_t sent_count = 0;
  uint32_t offset = 0;
  Report report(conf);

  if (signal(SIGINT, signal_handler) == SIG_ERR ||
      signal(SIGTERM, signal_handler) == SIG_ERR) {
    Util::eprint("failed to register signal");
    return;
  }

  if (conf->count_skip) {
    offset = conf->count_skip;
  } else {
    offset = read_offset(conf->offset_file);
  }
  if (!invoke(skip_data, this, static_cast<size_t>(offset))) {
    Util::eprint("failed to skip: ", offset);
  }

  report.start();

  while (!stop) {
    imessage_len = message_size;
    ret = invoke(get_next_data, this, imessage, imessage_len);
    if (ret == ControllerResult::Success) {
      omessage_len =
          conv->convert(imessage, imessage_len, omessage, message_size);
      write_offset(conf->offset_file, ++offset);
    } else if (ret == ControllerResult::Fail) {
      Util::eprint("failed to convert input data");
      break;
    } else if (ret == ControllerResult::No_more) {
      if (conf->input_type != InputType::Nic && conf->mode_grow) {
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

    if (!prod->produce(omessage)) {
      break;
    }

    report.process(imessage_len, omessage_len);

    sent_count = report.get_sent_count();
    Util::dprint(F, "[", sent_count, "]", " message : ", omessage);

    if (check_count(sent_count)) {
      break;
    }
  }

  report.end();
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

  if (conf->input.empty()) {
    return InputType::None;
  }

  DIR* dir_chk = nullptr;
  dir_chk = opendir(conf->input.c_str());
  if (dir_chk != nullptr) {
    closedir(dir_chk);
    return InputType::Dir;
  }

  ifstream ifs(conf->input, ios::binary);
  if (!ifs) {
    pcap_t* pcd_chk;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcd_chk =
        pcap_open_live(conf->input.c_str(), max_packet_length, 0, 1000, errbuf);
    if (pcd_chk != nullptr) {
      pcap_close(pcd_chk);
      return InputType::Nic;
    } else {
      throw runtime_error("failed to open input file: " + conf->input);
    }
  }

  ifs.seekg(0, ios::beg);
  unsigned char magic[4] = {0};
  ifs.read((char*)magic, sizeof(magic));

  if (memcmp(magic, mn_pcap_little_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_little_nano, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_nano, sizeof(magic)) == 0) {
    return InputType::Pcap;
  } else if (memcmp(magic, mn_pcapng_little, sizeof(magic)) == 0 ||
             memcmp(magic, mn_pcapng_big, sizeof(magic)) == 0) {
    return InputType::Pcapng;
  }

  return InputType::Log;
}

OutputType Controller::get_output_type() const
{
  if (conf->output.empty()) {
    return OutputType::Kafka;
  }

  if (conf->output == "none") {
    return OutputType::None;
  }

  return OutputType::File;
}

bool Controller::set_converter()
{
  conf->input_type = get_input_type();
  uint32_t l2_type;

  switch (conf->input_type) {
  case InputType::Nic:
    l2_type = open_nic(conf->input);
    conv = make_unique<PacketConverter>(l2_type);
    get_next_data = &Controller::get_next_nic;
    skip_data = &Controller::skip_null;
    Util::iprint("input=", conf->input, ", input type=NIC");
    break;
  case InputType::Pcap:
    l2_type = open_pcap(conf->input);
    conv = make_unique<PacketConverter>(l2_type);
    get_next_data = &Controller::get_next_pcap;
    skip_data = &Controller::skip_pcap;
    Util::iprint("input=", conf->input, ", input type=PCAP");
    break;
  case InputType::Log:
    open_log(conf->input);
    conv = make_unique<LogConverter>();
    get_next_data = &Controller::get_next_log;
    skip_data = &Controller::skip_log;
    Util::iprint("input=", conf->input, ", input type=LOG");
    break;
  case InputType::Dir:
    Util::iprint("input=", conf->input, ", input type=DIR");
    break;
  case InputType::None:
    conv = make_unique<NullConverter>();
    get_next_data = &Controller::get_next_null;
    skip_data = &Controller::skip_null;
    Util::iprint("input=", conf->input, ", input type=NONE");
    break;
  default:
    return false;
  }

  return true;
}

bool Controller::set_producer()
{
  conf->output_type = get_output_type();
  switch (conf->output_type) {
  case OutputType::Kafka:
    prod = make_unique<KafkaProducer>(conf);
    Util::iprint("output=", conf->output, ", output type=KAFKA");
    break;
  case OutputType::File:
    prod = make_unique<FileProducer>(conf);
    Util::iprint("output=", conf->output, ", output type=FILE");
    break;
  case OutputType::None:
    prod = make_unique<NullProducer>(conf);
    Util::iprint("output=", conf->output, ", output type=NONE");
    break;
  default:
    return false;
  }

  return true;
}

uint32_t Controller::open_nic(const std::string& devname)
{
  bpf_u_int32 net;
  bpf_u_int32 mask;
  struct bpf_program fp;
  char errbuf[PCAP_ERRBUF_SIZE];

  if (pcap_lookupnet(devname.c_str(), &net, &mask, errbuf) == -1) {
    throw runtime_error("failed to lookup the network: " + devname + " : " +
                        errbuf);
  }

  pcd = pcap_open_live(devname.c_str(), BUFSIZ, 0, -1, errbuf);

  if (pcd == nullptr) {
    throw runtime_error("failed to initialize the nic mode: " + devname +
                        " : " + errbuf);
  }

  if (pcap_setnonblock(pcd, false, errbuf) != 0) {
    Util::dprint(F, "non-blocking mode is set, it can cause cpu overhead");
  }

  if (pcap_compile(pcd, &fp, conf->packet_filter.c_str(), 0, net) == -1) {
    throw runtime_error("failed to compile the capture rules: " +
                        conf->packet_filter);
  }

  if (pcap_setfilter(pcd, &fp) == -1) {
    throw runtime_error("failed to set the capture rules: " +
                        conf->packet_filter);
  }

  Util::dprint(F, "capture rule setting completed" + conf->packet_filter);

  return pcap_datalink(pcd);
}

void Controller::close_nic()
{
  if (pcd != nullptr) {
    pcap_close(pcd);
  }
}

uint32_t Controller::open_pcap(const string& filename)
{
  pcapfile = fopen(filename.c_str(), "r");
  if (pcapfile == nullptr) {
    throw runtime_error("failed to open input file: " + filename);
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
    throw runtime_error("failed to open input file: " + filename);
  }
}

void Controller::close_log()
{
  if (logfile.is_open()) {
    logfile.close();
  }
}

uint32_t Controller::read_offset(const std::string& filename) const
{
  if (conf->input_type == InputType::Nic) {
    return 0;
  }

  string offset_str;
  uint32_t offset = 0;

  std::ifstream offset_file(filename);

  if (offset_file.is_open()) {
    getline(offset_file, offset_str);
    std::istringstream iss(offset_str);
    iss >> offset;
    iss.str("");
    offset_file.close();
    Util::iprint("the offset file exists. skips by offset value: ", offset);
  }

  return offset;
}

void Controller::write_offset(const std::string& filename,
                              uint32_t offset) const
{
  if (conf->input_type == InputType::Nic) {
    return;
  }

  std::ofstream offset_file(filename, ios::out | ios::trunc);

  if (offset_file.is_open()) {
    offset_file << offset;
    offset_file.close();
  }
}

ControllerResult Controller::get_next_nic(char* imessage, size_t& imessage_len)
{
  pcap_pkthdr* pkthdr;
  pcap_sf_pkthdr sfhdr;
  const u_char* pkt_data;
  size_t pp_len = sizeof(pcap_sf_pkthdr);
  auto ptr = reinterpret_cast<u_char*>(imessage);
  int res = 0;

  while (res == 0 && !stop) {
    res = pcap_next_ex(pcd, &pkthdr, &pkt_data);
  }
  if (res < 0 && !stop) {
    return ControllerResult::Fail;
  }
  if (stop == 1) {
    return ControllerResult::No_more;
  }
  sfhdr.caplen = pkthdr->caplen;
  sfhdr.len = pkthdr->len;
  sfhdr.ts.tv_sec = pkthdr->ts.tv_sec;
  // TODO: fix to satisfy both 32bit and 64bit systems
  // sfhdr.ts.tv_usec = pkthdr->??

  memcpy(ptr, reinterpret_cast<u_char*>(&sfhdr), pp_len);
  ptr += pp_len;
  memcpy(ptr, pkt_data, sfhdr.caplen);

  imessage_len = sfhdr.caplen + pp_len;

  return ControllerResult::Success;
}

ControllerResult Controller::get_next_pcap(char* imessage, size_t& imessage_len)
{
  size_t offset = 0;
  size_t pp_len = sizeof(pcap_sf_pkthdr);

  offset = fread(imessage, 1, pp_len, pcapfile);
  if (offset != pp_len) {
    fseek(pcapfile, -offset, SEEK_CUR);
    return ControllerResult::No_more;
  }

  auto* pp = reinterpret_cast<pcap_sf_pkthdr*>(imessage);
  if (pp->caplen > max_packet_length) {
    Util::eprint("The captured packet size is abnormally large: ", pp->caplen);
    return ControllerResult::Fail;
  }

  offset = fread(reinterpret_cast<char*>(imessage + static_cast<int>(pp_len)),
                 1, pp->caplen, pcapfile);
  if (offset != pp->caplen) {
    fseek(pcapfile, -(offset + pp_len), SEEK_CUR);
    return ControllerResult::No_more;
  }
  imessage_len = pp->caplen + pp_len;

  return ControllerResult::Success;
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
      return ControllerResult::No_more;
    }
    if (logfile.bad()) {
      return ControllerResult::Fail;
    }
    if (logfile.fail()) {
      Util::eprint("The message is truncated [", count, " line(", imessage_len,
                   " bytes)]");
      trunc = true;
      logfile.clear();
    }
  } else {
    imessage_len = strlen(imessage);
    if (trunc == true) {
      Util::eprint("The message is truncated [", count, " line(", imessage_len,
                   " bytes)]");
      trunc = false;
    }
  }

  return ControllerResult::Success;
}

ControllerResult Controller::get_next_null(char* imessage, size_t& imessage_len)
{
  static constexpr char sample_data[] =
      "1531980827 Ethernet2 a4:7b:2c:1f:eb:61 40:61:86:82:e9:26 IP 4 5 0 "
      "10240 "
      "58477 64 127 47112 59.7.91.107 123.141.115.52 ip_opt TCP 62555 80 "
      "86734452 2522990538 20 A 16425 7168 0";

  memcpy(imessage, sample_data, strlen(sample_data) + 1);
  imessage_len = strlen(imessage);

  return ControllerResult::Success;
}

std::string Controller::get_next_file(DIR* dir) const
{
  struct dirent* dirp = readdir(dir);
  if (dirp == nullptr) {
    return "";
  }
  return dirp->d_name;
}

bool Controller::skip_pcap(const size_t count_skip)
{
  if (count_skip == 0) {
    return true;
  }

  struct pcap_sf_pkthdr pp;
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
  if (count_skip == 0) {
    return true;
  }

  string tmp;
  size_t count = 0;

  while (count < count_skip) {
#if 0
    if (!logfile.getline(buf, 1)) {
      if (logfile.eof()) {
        return false;
      } else if (logfile.bad() || logfile.fail()) {
        return false;
      }
    }
#endif
    getline(logfile, tmp);
    count++;
  }
  if (logfile.eof() || logfile.bad() || logfile.fail()) {
    return false;
  }

  return true;
}

bool Controller::skip_null(const size_t count_skip) { return true; }

bool Controller::check_count(const size_t sent_count) const noexcept
{
  if (conf->count_send == 0 || sent_count < conf->count_send) {
    return false;
  }

  return true;
}

void Controller::signal_handler(int signal)
{
  if (pcd != nullptr) {
    pcap_breakloop(pcd);
  }
  stop = 1;
}

// vim: et:ts=2:sw=2
