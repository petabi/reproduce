#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>

#include <pcap/pcap.h>

#include "report.h"
#include "util.h"

using namespace std;

static constexpr double kbytes = 1024.0;
static constexpr double mbytes = kbytes * kbytes;

Report::Report(const Config* _conf) : conf(_conf) {}

Report::~Report()
{
  if (report_file.is_open()) {
    report_file.close();
  }
}

void Report::start(uint32_t id) noexcept
{
  if (!config_mode_eval(conf)) {
    return;
  }

  start_id = id;
  time_start = std::chrono::steady_clock::now();
}

void Report::process(const size_t bytes) noexcept
{
  if (!config_mode_eval(conf)) {
    return;
  }

  if (bytes > max_bytes) {
    max_bytes = bytes;
  } else if (bytes < min_bytes || min_bytes == 0) {
    min_bytes = bytes;
  }
  sum_bytes += bytes;

  process_cnt++;
}

void Report::skip(const size_t bytes) noexcept
{
  if (!config_mode_eval(conf)) {
    return;
  }

  skip_bytes += bytes;

  skip_cnt++;
}

template <typename T> inline auto PRINT_PRETTY_BYTES(T bytes) -> std::string
{
  double n = static_cast<double>(bytes) / mbytes;
  if (n < 1) {
    n = static_cast<double>(bytes) / kbytes;
    if (n < 1) {
      return std::string(std::to_string(bytes) + "B");
    } else {
      return std::string(std::to_string(n) + "KB");
    }
  }

  return std::string(std::to_string(n) + "MB");
}

auto Report::open_report_file(time_t launch_time) -> bool
{
  std::string filename;

  if (std::filesystem::is_directory("/report")) {
    filename = std::string("/report/") + config_kafka_topic(conf);
  } else {
    filename = config_kafka_topic(conf);
  }

  report_file.open(filename, ios::out | ios::app);

  return report_file.is_open();
}

void Report::end(uint32_t id, time_t launch_time) noexcept
{
  constexpr int arrange_var = 28;
  if (!config_mode_eval(conf)) {
    return;
  }
  if (!open_report_file(launch_time)) {
    return;
  }

  end_id = id;
  time_now = std::chrono::steady_clock::now();
  time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(
      time_now - time_start);

  if (process_cnt) {
    avg_bytes = static_cast<double>(sum_bytes) / process_cnt;
  }

  report_file.precision(2);
  report_file << fixed;
  report_file << "--------------------------------------------------\n";

  time_t current_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  report_file << left << setw(arrange_var)
              << "Time: " << std::ctime(&current_time);

  struct stat st;
  size_t process_bytes = 0;
  switch (config_input_type(conf)) {
  case InputType::Pcap:
  case InputType::Pcapng:
    if (config_input_type(conf) == InputType::Pcap) {
      report_file << left << setw(arrange_var) << "Input(PCAP): ";
    } else {
      report_file << left << setw(arrange_var) << "Input(PCAPNG): ";
    }
    report_file << config_input(conf);
    process_bytes = sum_bytes + sizeof(struct pcap_file_header);
    break;
  case InputType::Log:
    report_file << left << setw(arrange_var)
                << "Input(LOG): " << config_input(conf);
    // add 1 byte newline character per line
    process_bytes = sum_bytes + process_cnt;
    break;
  case InputType::Nic:
    process_bytes = sum_bytes - (sizeof(pcap_pkthdr) * process_cnt);
    report_file << left << setw(arrange_var)
                << "Input(NIC): " << config_input(conf);
    break;
  default:
    break;
  }
  if (config_input_type(conf) != InputType::Nic &&
      stat(config_input(conf), &st) != -1) {
    report_file << "(" << PRINT_PRETTY_BYTES(st.st_size) << ")\n";
  } else {
    report_file << '\n';
  }

  report_file << left << setw(arrange_var)
              << "Datasource ID: " << (int)config_datasource_id(conf) << "\n";
  report_file << left << setw(arrange_var) << "Input ID: " << start_id << " ~ "
              << end_id << "\n";

  switch (config_output_type(conf)) {
  case OutputType::File:
    report_file << left << setw(arrange_var)
                << "Output(FILE): " << config_output(conf);
    if (stat(config_input(conf), &st) != -1) {
      report_file << "(" << PRINT_PRETTY_BYTES(st.st_size) << ")\n";
    } else {
      report_file << "(invalid)\n";
    }
    break;
  case OutputType::Kafka:
    report_file << left << setw(arrange_var)
                << "Output(KAFKA): " << config_kafka_broker(conf) << "("
                << config_kafka_topic(conf) << ")\n";
    break;
  case OutputType::None:
    report_file << "Output(NONE): \n";
    break;
  }

  report_file << left << setw(arrange_var)
              << "Statistics(Min/Max/Avg): " << min_bytes << "/" << max_bytes
              << "/" << avg_bytes << "bytes\n";
  report_file << left << setw(arrange_var) << "Process Count: " << process_cnt
              << "(" << PRINT_PRETTY_BYTES(process_bytes) << ")\n";
  report_file << left << setw(arrange_var) << "Skip Count: " << skip_cnt << "("
              << PRINT_PRETTY_BYTES(skip_bytes) << ")\n";
  report_file << left << setw(arrange_var)
              << "Elapsed Time: " << time_diff.count() << "s\n";
  report_file << left << setw(arrange_var) << "Performance: "
              << PRINT_PRETTY_BYTES(static_cast<double>(process_bytes) /
                                    time_diff.count())
              << "ps\n";

  if (report_file.is_open()) {
    report_file.close();
  }
}

// vim: et:ts=2:sw=2
