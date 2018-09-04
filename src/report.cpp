#include <iostream>
#include <sys/stat.h>

#include "report.h"

using namespace std;

static constexpr double KBYTE = 1024.0;
static constexpr double MBYTE = KBYTE * KBYTE;
static constexpr double KPACKET = 1000.0;
static constexpr double MPACKET = KPACKET * KPACKET;

// Report::Report(Config _conf) : conf(_conf) {}

void Report::start() noexcept
{
  if (!conf.mode_eval) {
    return;
  }

  time_start = clock();
}

void Report::process(int length) noexcept
{
  if (length > 0) {
    sent_byte += length;
  }
  sent_count++;

  if (!conf.mode_eval) {
    return;
  }

  time_now = clock();
  time_diff = (double)(time_now - time_start) / CLOCKS_PER_SEC;

  if (time_diff) {
    perf_kbps = (double)sent_byte / KBYTE / time_diff;
    perf_kpps = (double)sent_count / KPACKET / time_diff;
  }
}

void Report::end() const noexcept
{
  if (!conf.mode_eval) {
    return;
  }

  cout.precision(2);
  cout << fixed;
  cout << "--------------------------------------------------\n";
  struct stat st;
  switch (conf.input_type) {
  case InputType::PCAP:
    cout << "Input(PCAP)\t: " << conf.input;
    if (stat(conf.input.c_str(), &st) != -1) {
      cout << "(" << (double)st.st_size / MBYTE << "M)\n";
    } else {
      cout << '\n';
    }
    break;
  case InputType::LOG:
    cout << "Input(LOG)\t: " << conf.input;
    if (stat(conf.input.c_str(), &st) != -1) {
      cout << "(" << (double)st.st_size / MBYTE << "M)\n";
    } else {
      cout << '\n';
    }
    break;
  case InputType::NONE:
    cout << "Input(NONE)\t: \n";
    break;
  default:
    break;
  }

  switch (conf.output_type) {
  case OutputType::FILE:
    cout << "Output(FILE)\t: " << conf.output;
    if (stat(conf.input.c_str(), &st) != -1) {
      cout << "(" << (double)st.st_size / MBYTE << "M)\n";
    } else {
      cout << "(invalid)\n";
    }
    break;
  case OutputType::KAFKA:
    cout << "Output\t\t: KAFKA(" << conf.broker << " | " << conf.topic << ")\n";
    break;
  case OutputType::NONE:
    cout << "Output(NONE)\t: \n";
    break;
  }
  cout << "Sent Bytes\t: " << sent_byte << "(" << (double)sent_byte / MBYTE
       << "M)\n";
  cout << "Sent Count\t: " << sent_count << "(" << (double)sent_count / MPACKET
       << "M)\n";
  cout << "Fail Count\t: " << fail_count << "(" << (double)fail_count / MPACKET
       << "M)\n";
  // TODO: converted data info
  cout << "Elapsed Time\t: " << time_diff << "s\n";
  cout << "Performance\t: " << perf_kbps / KBYTE << "MBps/" << perf_kpps
       << "Kpps\n";
  cout << "--------------------------------------------------\n";
}

void Report::fail() noexcept { fail_count++; }

size_t Report::get_sent_count() const noexcept { return sent_count; }

// vim: et:ts=2:sw=2
