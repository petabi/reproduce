#include <iomanip>
#include <iostream>
#include <sys/stat.h>

#include <pcap/pcap.h>

#include "report.h"

using namespace std;

static constexpr double kbytes = 1024.0;
static constexpr double mbytes = kbytes * kbytes;
constexpr char filename[] = "report.txt";

Report::Report(shared_ptr<Config> _conf) : conf(move(_conf)) {}

Report::~Report()
{
  if (report_file.is_open()) {
    report_file.close();
  }
}

void Report::start(size_t id) noexcept
{
  if (!conf->mode_eval) {
    return;
  }

  start_id = id;
  time_start = std::chrono::steady_clock::now();
}

void Report::process(const size_t bytes) noexcept
{
  if (!conf->mode_eval) {
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
  if (!conf->mode_eval) {
    return;
  }

  skip_bytes += bytes;

  skip_cnt++;
}

template <typename T> inline std::string PRINT_PRETTY_BYTES(T bytes)
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
#if 0
inline std::string PRINT_PRETTY_BYTES(size_t bytes)
{
  double n = static_cast<double>(bytes) / mbytes;
  if (n < 1) {
    n = static_cast<double>(bytes) / kbytes;
    if (n < 1) {
      std::string(std::to_string(bytes) + "B");
    } else {
      return std::string(std::to_string(n) + "KB");
    }
  }

  return std::string(std::to_string(n) + "MB");
}
#endif
void Report::end(size_t id) noexcept
{
  constexpr int arrange_var = 28;
  if (!conf->mode_eval) {
    return;
  }

  report_file.open(filename, ios::out | ios::app);

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

  time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  report_file << left << setw(arrange_var) << "Time: " << std::ctime(&current_time);


  struct stat st;
  size_t process_bytes = 0;
  switch (conf->input_type) {
  case InputType::Pcap:
    report_file << left << setw(arrange_var) << "Input(PCAP): " << conf->input;
    process_bytes = sum_bytes + sizeof(struct pcap_file_header);
    break;
  case InputType::Log:
    report_file << left << setw(arrange_var) << "Input(LOG): " << conf->input;
    // add 1 byte newline character per line
    process_bytes = sum_bytes + process_cnt;
    break;
  case InputType::Nic:
    process_bytes = sum_bytes - (sizeof(pcap_pkthdr) * process_cnt);
    report_file << left << setw(arrange_var) << "Input(NIC): " << conf->input;
    break;
  default:
    break;
  }
  if (conf->input_type != InputType::Nic &&
      stat(conf->input.c_str(), &st) != -1) {
    report_file << "(" << PRINT_PRETTY_BYTES(st.st_size) << ")\n";
  } else {
    report_file << '\n';
  }

  report_file << left << setw(arrange_var) << "Datasource ID: " << conf->datasource_id << "\n";
  report_file << left << setw(arrange_var) << "Input ID: " << (start_id & 0x0000FFFFFFFFFFFF) << " ~ "
              << (end_id & 0x0000FFFFFFFFFFFF) << "\n";

  switch (conf->output_type) {
  case OutputType::File:
    report_file << left << setw(arrange_var)
                << "Output(FILE): " << conf->output;
    if (stat(conf->input.c_str(), &st) != -1) {
      report_file << "(" << PRINT_PRETTY_BYTES(st.st_size) << ")\n";
    } else {
      report_file << "(invalid)\n";
    }
    break;
  case OutputType::Kafka:
    report_file << left << setw(arrange_var)
                << "Output(KAFKA): " << conf->kafka_broker << "("
                << conf->kafka_topic << ")\n";
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
