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
#include "event.h"

using namespace std;

static constexpr size_t max_packet_length = 65535;
static constexpr size_t message_size = 102400;
static constexpr size_t kafka_buffer_safety_gap = 1024;

atomic_bool stop = false;
pcap_t* Controller::pcd = nullptr;

Controller::Controller(Config const& _conf)
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
  std::vector<string> files, read_files;

  do {
    files = traverse_directory(dir_path, conf->file_prefix, read_files);

    if (files.empty()) {
      if (conf->mode_polling_dir) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        continue;
      } else {
        throw runtime_error("no input files");
      }
    }

    sort(files.begin(), files.end());

    for (const auto& file : files) {
      conf->input = file;

      if (!set_converter()) {
        throw runtime_error("failed to set the converter");
      }

      this->run();

      close_pcap();
      close_log();
    }

    if (conf->mode_polling_dir) {
      for (auto& f : files) {
        read_files.push_back(f);
      }
      files.clear();
    } else {
      break;
    }
  } while (true);
}

void Controller::run_single()
{
  std::array<char, message_size> imessage{0};
  size_t imessage_len = 0;
  size_t conv_cnt = 0;
  uint32_t offset = 0;
  Report report(conf);

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

  // set initial sequence number: begin at 1 or skip_count + 1
  // or user defined seq_no
  if (seq_no == 1) {
    if (offset) {
      seq_no = static_cast<uint32_t>(offset) + 1;
    }

    // if user set the initial seq_no, the skip count will not be used for
    // numbering the seq_no
    if (conf->initial_seq_no) {
      seq_no = conf->initial_seq_no;
    }
  }

  report.start(get_seq_no(0));
  auto msg = forward_mode_new();
  forward_mode_set_tag(msg, "REproduce");
  auto buf = serialization_buffer_new();

  while (!stop) {
    imessage_len = message_size;
    GetData::Status gstat =
        invoke(get_next_data, this, imessage.data(), imessage_len);
    if (gstat == GetData::Status::Success) {
      if ((forward_mode_serialized_len(msg) + imessage_len) >=
          prod->get_max_bytes() - kafka_buffer_safety_gap) {
        forward_mode_serialize(msg, buf);
        if (!prod->produce(
                reinterpret_cast<const char*>(serialization_buffer_data(buf)),
                serialization_buffer_len(buf), true)) {
          break;
        }
        forward_mode_clear(msg);
        forward_mode_set_tag(msg, "REproduce");
      }

      Conv::Status cstat =
          conv->convert(event_id(), imessage.data(), imessage_len, msg);
      seq_no++;

      if (cstat != Conv::Status::Success) {
        // TODO: handling exceptions due to convert failures
        report.skip(imessage_len);
      }
    } else if (gstat == GetData::Status::Fail) {
      Util::eprint("failed to convert input data");
      break;
    } else if (gstat == GetData::Status::No_more) {
      if (conf->input_type != InputType::Nic && conf->mode_grow &&
          !conf->mode_polling_dir) {
        // TODO: optimize time interval
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
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
    conv->update_pack_message(event_id(), pm.fm, nullptr, 0);
  }
  */

  if (forward_mode_size(msg) > 0) {
    forward_mode_serialize(msg, buf);
    prod->produce(reinterpret_cast<const char*>(serialization_buffer_data(buf)),
                  serialization_buffer_len(buf), true);
  }
  forward_mode_free(msg);
  write_offset(conf->input + "_" + conf->offset_prefix, offset + conv_cnt);
  report.end(get_seq_no(-1), launch_time);
}

auto Controller::get_input_type() const -> InputType
{
  // pcap: c3d4 a1b2
  // pcap-ng: 0d0a 0a0d
  constexpr std::array<unsigned char, 4> mn_pcap_little_micro{0xd4, 0xc3, 0xb2,
                                                              0xa1};
  constexpr std::array<unsigned char, 4> mn_pcap_big_micro = {0xa1, 0xb2, 0xc3,
                                                              0xd4};
  constexpr std::array<unsigned char, 4> mn_pcap_little_nano = {0x4d, 0x3c,
                                                                0xb2, 0xa1};
  constexpr std::array<unsigned char, 4> mn_pcap_big_nano = {0xa1, 0xb2, 0x3c,
                                                             0x4d};
  constexpr std::array<unsigned char, 4> mn_pcapng_big = {0x1A, 0x2B, 0x3C,
                                                          0x4D};
  constexpr std::array<unsigned char, 4> mn_pcapng_little = {0x0a, 0x0d, 0x0d,
                                                             0x0a};

  DIR* dir_chk = nullptr;
  dir_chk = opendir(conf->input.c_str());
  if (dir_chk != nullptr) {
    closedir(dir_chk);
    return InputType::Dir;
  }

  ifstream ifs(conf->input, ios::binary);
  if (!ifs) {
    pcap_t* pcd_chk;
    std::array<char, PCAP_ERRBUF_SIZE> errbuf{0};
    pcd_chk = pcap_open_live(conf->input.c_str(), max_packet_length, 0, 1000,
                             errbuf.data());
    if (pcd_chk != nullptr) {
      pcap_close(pcd_chk);
      return InputType::Nic;
    } else {
      throw runtime_error("failed to open input file: " + conf->input);
    }
  }

  ifs.seekg(0, ios::beg);
  std::array<unsigned char, 4> magic{0};
  ifs.read((char*)magic.data(), magic.size());

  if (magic == mn_pcap_little_micro || magic == mn_pcap_big_micro ||
      magic == mn_pcap_little_nano || magic == mn_pcap_big_nano) {
    return InputType::Pcap;
  } else if (magic == mn_pcapng_little || magic == mn_pcapng_big) {
    return InputType::Pcapng;
  }

  return InputType::Log;
}

auto Controller::get_output_type() const -> OutputType
{
  if (conf->output.empty()) {
    return OutputType::Kafka;
  }

  if (conf->output == "none") {
    return OutputType::None;
  }

  return OutputType::File;
}

auto Controller::set_converter() -> bool
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
  case InputType::Pcapng:
    l2_type = open_pcap(conf->input);
    conv = make_unique<PacketConverter>(l2_type, launch_time);
    if (!conf->pattern_file.empty()) {
      if (conv->get_matcher() == nullptr) {
        conv->set_matcher(conf->pattern_file, Mode::BLOCK);
      }
    }
    get_next_data = &Controller::get_next_pcap;
    skip_data = &Controller::skip_pcap;
    if (conf->input_type == InputType::Pcap)
      Util::iprint("input=", conf->input, ", input type=PCAP");
    else
      Util::iprint("input=", conf->input, ", input type=PCAPNG");

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

auto Controller::set_producer() -> bool
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

auto Controller::open_nic(const std::string& devname) -> uint32_t
{
  bpf_u_int32 net;
  bpf_u_int32 mask;
  struct bpf_program fp;
  std::array<char, PCAP_ERRBUF_SIZE> errbuf{0};

  if (pcap_lookupnet(devname.c_str(), &net, &mask, errbuf.data()) == -1) {
    throw runtime_error("failed to lookup the network: " + devname + ": " +
                        errbuf.data());
  }

  pcd = pcap_open_live(devname.c_str(), BUFSIZ, 0, -1, errbuf.data());

  if (pcd == nullptr) {
    throw runtime_error("failed to initialize the nic mode: " + devname + ": " +
                        errbuf.data());
  }

  if (pcap_setnonblock(pcd, false, errbuf.data()) != 0) {
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

auto Controller::open_pcap(const string& filename) -> uint32_t
{
  struct bpf_program fp;
  std::array<char, PCAP_ERRBUF_SIZE> errbuf{0};

  pcd = pcap_open_offline(filename.c_str(), errbuf.data());

  if (pcd == nullptr) {
    throw runtime_error("failed to initialize the pcap file mode: " + filename +
                        ": " + errbuf.data());
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

auto Controller::read_offset(const std::string& filename) const -> uint32_t
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

auto Controller::get_next_nic(char* imessage, size_t& imessage_len)
    -> GetData::Status
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

auto Controller::get_next_pcap(char* imessage, size_t& imessage_len)
    -> GetData::Status
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

auto Controller::get_next_log(char* imessage, size_t& imessage_len)
    -> GetData::Status
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

auto Controller::traverse_directory(std::string& _path,
                                    const std::string& _prefix,
                                    std::vector<std::string>& read_files)
    -> std::vector<std::string>
{
  std::vector<std::string> _files;

  for (auto entry = std::filesystem::recursive_directory_iterator(_path);
       entry != std::filesystem::recursive_directory_iterator(); ++entry) {

    if (_prefix.length() > 0 && entry->path().filename().string().compare(
                                    0, _prefix.length(), _prefix) != 0) {
      continue;
    }

    if (std::filesystem::is_regular_file(entry->path())) {
      auto it = find(read_files.begin(), read_files.end(), entry->path());
      if (it == read_files.end()) {
        _files.push_back(entry->path());
      }
    }
  }

  return _files;
}

auto Controller::skip_pcap(const size_t count_skip) -> bool
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

auto Controller::skip_log(const size_t count_skip) -> bool
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

auto Controller::skip_null(const size_t count_skip) -> bool { return true; }

auto Controller::check_count(const size_t sent_cnt) const noexcept -> bool
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
