#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <thread>
#include <vector>

#include <pcap/pcap.h>

#include "controller.h"
#include "forward_proto.h"

using namespace std;

static constexpr size_t max_packet_length = 65535;
static constexpr size_t message_size = 102400;
atomic_bool stop = false;
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
  std::vector<string> files = traverse_directory(dir_path, conf->file_prefix);

  if (!files.empty()) {
    sort(files.begin(), files.end());
  } else {
    throw runtime_error("no file exists");
  }

  for (const auto& file : files) {
    conf->input = file;

    if (!set_converter()) {
      throw runtime_error("failed to set the converter");
    }

    this->run();

    close_pcap();
    close_log();
  }
}

void Controller::run_single()
{
  char imessage[message_size];
  size_t imessage_len = 0;
  size_t conv_cnt = 0;
  uint32_t offset = 0;
  Report report(conf);
  std::stringstream ss;

  if (signal(SIGINT, signal_handler) == SIG_ERR ||
      signal(SIGTERM, signal_handler) == SIG_ERR ||
      signal(SIGUSR1, signal_handler) == SIG_ERR) {
    Util::eprint("failed to register signal");
    return;
  }

  if (conf->count_skip) {
    offset = conf->count_skip;
  } else {
    offset = read_offset(conf->input + "_" + conf->offset_prefix);
  }

  if (!invoke(skip_data, this, static_cast<size_t>(offset))) {
    Util::eprint("failed to skip: ", offset);
  }

  if (offset > 0) {
    read_count = static_cast<uint64_t>(offset);
  }

  report.start(read_count + 1);
  PackMsg pm;
  pm.set_max_bytes(prod->get_max_bytes());
  pm.tag("REproduce");
  pm.option("option", "optional");

  while (!stop) {
    imessage_len = message_size;
    GetData::Status gstat = invoke(get_next_data, this, imessage, imessage_len);
    if (gstat == GetData::Status::Success) {
      if ((pm.get_bytes() + imessage_len) >= pm.get_max_bytes()) {
        pm.pack(ss);
        if (!prod->produce(ss.str(), true)) {
          break;
        }
        pm.clear();
        ss.str("");
        pm.tag("REproduce");
        pm.option("option", "optional");
      }
      read_count++;
      Conv::Status cstat = conv->convert(read_count | conf->datasource_id,
                                         imessage, imessage_len, pm);
      if (cstat != Conv::Status::Success) {
        // TODO: handling exceptions due to convert failures
        report.skip(imessage_len);
      }
    } else if (gstat == GetData::Status::Fail) {
      Util::eprint("failed to convert input data");
      break;
    } else if (gstat == GetData::Status::No_more) {
      if (conf->input_type != InputType::Nic && conf->mode_grow) {
        // TODO: optimize time interval
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }
      break;
    } else {
      break;
    }

    conv_cnt++;
    report.process(imessage_len);

    if (check_count(conv_cnt)) {
      break;
    }
  }
  // only use when processing session data
  /*
  if (conv->remaining_data()) {
    conv->update_pack_message(read_count | conf->datasource_id, pm, nullptr, 0);
  }
  */

  if (pm.get_entries() > 0) {
    pm.pack(ss);
    prod->produce(ss.str(), true);
    pm.clear();
    ss.str("");
  }
  write_offset(conf->input + "_" + conf->offset_prefix, offset + conv_cnt);
  report.end(read_count, launch_time);
}

InputType Controller::get_input_type() const
{
  const unsigned char mn_pcap_little_micro[4] = {0xd4, 0xc3, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_micro[4] = {0xa1, 0xb2, 0xc3, 0xd4};
  const unsigned char mn_pcap_little_nano[4] = {0x4d, 0x3c, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_nano[4] = {0xa1, 0xb2, 0x3c, 0x4d};
  const unsigned char mn_pcapng_little[4] = {0x4D, 0x3C, 0x2B, 0x1A};
  const unsigned char mn_pcapng_big[4] = {0x1A, 0x2B, 0x3C, 0x4D};

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

  if (launch_time == 0) {
    launch_time = time(nullptr);
  }

  switch (conf->input_type) {
  case InputType::Nic:
    l2_type = open_nic(conf->input);
    conv = make_unique<PacketConverter>(l2_type, launch_time);
    if (!conf->pattern_file.empty()) {
      if (conv->get_matcher() == nullptr) {
        conv->set_matcher(conf->pattern_file, Mode::BLOCK);
      }
    }
    get_next_data = &Controller::get_next_nic;
    skip_data = &Controller::skip_null;
    Util::iprint("input=", conf->input, ", input type=NIC");
    if (conv->get_matcher() != nullptr) {
      Util::iprint("pattern file=", conf->pattern_file);
    }
    conv->set_allowed_entropy_ratio(conf->entropy_ratio);
    break;
  case InputType::Pcap:
    l2_type = open_pcap(conf->input);
    conv = make_unique<PacketConverter>(l2_type, launch_time);
    if (!conf->pattern_file.empty()) {
      if (conv->get_matcher() == nullptr) {
        conv->set_matcher(conf->pattern_file, Mode::BLOCK);
      }
    }
    get_next_data = &Controller::get_next_pcap;
    skip_data = &Controller::skip_pcap;
    Util::iprint("input=", conf->input, ", input type=PCAP");
    if (conv->get_matcher() != nullptr) {
      Util::iprint("pattern file=", conf->pattern_file);
    }
    conv->set_allowed_entropy_ratio(conf->entropy_ratio);
    break;
  case InputType::Log:
    open_log(conf->input);
    conv = make_unique<LogConverter>();
    if (!conf->pattern_file.empty()) {
      if (conv->get_matcher() == nullptr) {
        conv->set_matcher(conf->pattern_file, Mode::BLOCK);
      }
    }
    get_next_data = &Controller::get_next_log;
    skip_data = &Controller::skip_log;
    Util::iprint("input=", conf->input, ", input type=LOG");
    if (conv->get_matcher() != nullptr) {
      Util::iprint("pattern file=", conf->pattern_file);
    }
    break;
  case InputType::Dir:
    Util::iprint("input=", conf->input, ", input type=DIR");
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
    throw runtime_error("failed to lookup the network: " + devname + ": " +
                        errbuf);
  }

  pcd = pcap_open_live(devname.c_str(), BUFSIZ, 0, -1, errbuf);

  if (pcd == nullptr) {
    throw runtime_error("failed to initialize the nic mode: " + devname + ": " +
                        errbuf);
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

  Util::dprint(F, "capture rule setting completed: " + conf->packet_filter);

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
  struct bpf_program fp;
  char errbuf[PCAP_ERRBUF_SIZE];

  pcd = pcap_open_offline(filename.c_str(), errbuf);

  if (pcd == nullptr) {
    throw runtime_error("failed to initialize the pcap file mode: " + filename +
                        ": " + errbuf);
  }

  if (pcap_compile(pcd, &fp, conf->packet_filter.c_str(), 0, 0) == -1) {
    throw runtime_error("failed to compile the capture rules: " +
                        conf->packet_filter);
  }

  if (pcap_setfilter(pcd, &fp) == -1) {
    throw runtime_error("failed to set the capture rules: " +
                        conf->packet_filter);
  }

  Util::dprint(F, "capture rule setting completed: " + conf->packet_filter);

  return pcap_datalink(pcd);
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
  if (conf->offset_prefix.empty() || conf->input_type == InputType::Nic) {
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
  if (conf->offset_prefix.empty() || conf->input_type == InputType::Nic) {
    return;
  }

  std::ofstream offset_file(filename, ios::out | ios::trunc);

  if (offset_file.is_open()) {
    offset_file << offset;
    offset_file.close();
  }
}

GetData::Status Controller::get_next_nic(char* imessage, size_t& imessage_len)
{
  pcap_pkthdr* pkthdr;
  pcap_sf_pkthdr sfhdr;
  const u_char* pkt_data;
  size_t pp_len = sizeof(pcap_sf_pkthdr);
  auto ptr = reinterpret_cast<u_char*>(imessage);
  int res = 0;

  do {
    res = pcap_next_ex(pcd, &pkthdr, &pkt_data);
  } while (res == 0 && !stop);
  if (stop) {
    return GetData::Status::No_more;
  } else if (res < 0) {
    return GetData::Status::Fail;
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

  return GetData::Status::Success;
}

GetData::Status Controller::get_next_pcap(char* imessage, size_t& imessage_len)
{
  pcap_pkthdr* pkthdr;
  pcap_sf_pkthdr sfhdr;
  const u_char* pkt_data;
  size_t pp_len = sizeof(pcap_sf_pkthdr);
  auto ptr = reinterpret_cast<u_char*>(imessage);
  int res = 0;

  res = pcap_next_ex(pcd, &pkthdr, &pkt_data);
  if (res == -1) {
    return GetData::Status::Fail;
  }
  if (res == -2) {
    return GetData::Status::No_more;
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

  return GetData::Status::Success;
}

GetData::Status Controller::get_next_log(char* imessage, size_t& imessage_len)
{
  static size_t count = 0;
  static bool trunc = false;
  string line;

  count++;
  if (!logfile.getline(imessage, imessage_len)) {
    if (logfile.eof()) {
      logfile.clear();
      return GetData::Status::No_more;
    }
    if (logfile.bad()) {
      return GetData::Status::Fail;
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

  return GetData::Status::Success;
}

std::vector<std::string>
Controller::traverse_directory(std::string& _path, const std::string& _prefix)
{
  std::vector<std::string> _files;

  for (auto entry = std::filesystem::recursive_directory_iterator(_path);
       entry != std::filesystem::recursive_directory_iterator(); ++entry) {

    if (_prefix.length() > 0 && entry->path().filename().string().compare(
                                    0, _prefix.length(), _prefix) != 0) {
      continue;
    }

    if (std::filesystem::is_regular_file(entry->path())) {
      _files.push_back(entry->path());
    }
  }

  return _files;
}

bool Controller::skip_pcap(const size_t count_skip)
{
  if (count_skip == 0) {
    return true;
  }

  pcap_pkthdr* pkthdr;
  const u_char* pkt_data;
  int res = 0;
  size_t count = 0;

  while (count < count_skip) {
    res = pcap_next_ex(pcd, &pkthdr, &pkt_data);
    if (res < 0) {
      return false;
    }
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

bool Controller::check_count(const size_t sent_cnt) const noexcept
{
  if (conf->count_send == 0 || sent_cnt < conf->count_send) {
    return false;
  }

  return true;
}

void Controller::signal_handler(int signal)
{
  if (signal == SIGINT || signal == SIGTERM) {
    if (pcd != nullptr) {
      pcap_breakloop(pcd);
    }
    stop = true;
  } else if (signal == SIGUSR1) {
    // TODO: processing according to the signal generated by confd script
    Util::eprint("detect config change!!!");
  }
}

// vim: et:ts=2:sw=2
