#include <iostream>
#include <sys/stat.h>

#include "report.h"

using namespace std;

static constexpr double kbytes = 1024.0;
static constexpr double mbytes = kbytes * kbytes;

Report::Report(shared_ptr<Config> _conf) : conf(move(_conf)) {}

void Report::start() noexcept
{
  if (!conf->mode_eval) {
    return;
  }

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

  count++;
}

void Report::end() noexcept
{
  if (!conf->mode_eval) {
    return;
  }

  time_now = std::chrono::steady_clock::now();
  time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(
      time_now - time_start);

  if (time_diff.count()) {
    perf_kbps = static_cast<double>(sum_bytes) / kbytes / time_diff.count();
  }
  if (count) {
    avg_bytes = static_cast<double>(sum_bytes) / count;
  }

  cout.precision(2);
  cout << fixed;
  cout << "--------------------------------------------------\n";
  struct stat st;
  switch (conf->input_type) {
  case InputType::Pcap:
    cout << "Input(PCAP)\t: " << conf->input;
    if (stat(conf->input.c_str(), &st) != -1) {
      cout << "(" << static_cast<double>(st.st_size) / mbytes << "M)\n";
    } else {
      cout << '\n';
    }
    break;
  case InputType::Log:
    cout << "Input(LOG)\t: " << conf->input;
    if (stat(conf->input.c_str(), &st) != -1) {
      cout << "(" << static_cast<double>(st.st_size) / mbytes << "M)\n";
    } else {
      cout << '\n';
    }
    break;
  case InputType::Nic:
    cout << "Input(NIC)\t: " << conf->input << '\n';
    break;
  default:
    break;
  }
  switch (conf->output_type) {
  case OutputType::File:
    cout << "Output(FILE)\t: " << conf->output;
    if (stat(conf->input.c_str(), &st) != -1) {
      cout << "(" << static_cast<double>(st.st_size) / mbytes << "M)\n";
    } else {
      cout << "(invalid)\n";
    }
    break;
  case OutputType::Kafka:
    cout << "Output(KAFKA)\t: " << conf->kafka_broker << "("
         << conf->kafka_topic << ")\n";
    break;
  case OutputType::None:
    cout << "Output(NONE)\t: \n";
    break;
  }
  cout << "Process Count\t: " << count << "(" << sum_bytes << "bytes)\n";
  cout << "Input Data\t: " << min_bytes << "/" << max_bytes << "/" << avg_bytes
       << "(Min/Max/Avg)\n";
  cout << "Elapsed Time\t: " << time_diff.count() << "s\n";
  cout << "Performance\t: " << static_cast<double>(st.st_size) / mbytes
       << "MBps\n";
  cout << "--------------------------------------------------\n";
}

// vim: et:ts=2:sw=2
