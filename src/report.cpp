#include <iostream>
#include <sys/stat.h>

#include "report.h"

using namespace std;

static constexpr double kbytes = 1024.0;
static constexpr double mbytes = kbytes * kbytes;
static constexpr double kpackets = 1000.0;
static constexpr double mpackets = kpackets * kpackets;

Report::Report(shared_ptr<Config> _conf) : conf(move(_conf)) {}

void Report::start() noexcept
{
  if (!conf->mode_eval) {
    return;
  }

  time_start = std::chrono::steady_clock::now();
}

void Report::process(const size_t orig_length,
                     const size_t sent_length) noexcept
{
  if (orig_length > orig_byte_max) {
    orig_byte_max = orig_length;
  } else if (orig_length < orig_byte_min || orig_byte_min == 0) {
    orig_byte_min = orig_length;
  }
  orig_byte += orig_length;

  if (sent_length > sent_byte_max) {
    sent_byte_max = sent_length;
  } else if (sent_length < sent_byte_min || sent_byte_min == 0) {
    sent_byte_min = sent_length;
  }
  sent_byte += sent_length;

  sent_count++;
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
    perf_kbps = static_cast<double>(sent_byte) / kbytes / time_diff.count();
    perf_kpps = static_cast<double>(sent_count) / kpackets / time_diff.count();
  }
  if (sent_count) {
    orig_byte_avg = static_cast<double>(orig_byte) / sent_count;
    sent_byte_avg = static_cast<double>(sent_byte) / sent_count;
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
  case InputType::None:
    cout << "Input(NONE)\t: \n";
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
  cout << "Input Length\t: " << orig_byte_min << "/" << orig_byte_max << "/"
       << orig_byte_avg << "(Min/Max/Avg)\n";
  cout << "Output Length\t: " << sent_byte_min << "/" << sent_byte_max << "/"
       << sent_byte_avg << "(Min/Max/Avg)\n";
  cout << "Sent Bytes\t: " << sent_byte << "("
       << static_cast<double>(sent_byte) / mbytes << "M)\n";
  cout << "Sent Count\t: " << sent_count << "("
       << static_cast<double>(sent_count) / mpackets << "M)\n";
  cout << "Fail Count\t: " << fail_count << "("
       << static_cast<double>(fail_count) / mpackets << "M)\n";
  cout << "Elapsed Time\t: " << time_diff.count() << "s\n";
  cout << "Performance\t: " << perf_kbps / kbytes << "MBps/" << perf_kpps
       << "Kpps\n";
  cout << "--------------------------------------------------\n";
}

void Report::fail() noexcept { fail_count++; }

size_t Report::get_sent_count() const noexcept { return sent_count; }

// vim: et:ts=2:sw=2
