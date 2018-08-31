#include <iostream>
#include <sys/stat.h>

#include "report.h"

using namespace std;

static constexpr double KBYTE = 1024.0;
static constexpr double MBYTE = KBYTE * KBYTE;
static constexpr double KPACKET = 1000.0;
static constexpr double MPACKET = KPACKET * KPACKET;

Report::Report(Config& _conf) : conf(_conf) {}

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

  struct stat st;

  cout.precision(2);
  cout << fixed;
  cout << "--------------------------------------------------\n";
  if (stat(conf.input.c_str(), &st) != -1) {
    cout << "Input File  : " << conf.input << "(" << (double)st.st_size / MBYTE
         << "M)\n";
  } else {
    cout << "Input File  : invalid\n";
  }
  cout << "Sent Bytes  : " << sent_byte << "(" << (double)sent_byte / MBYTE
       << "M)\n";
  cout << "Sent Packets: " << sent_count << "(" << (double)sent_count / MPACKET
       << "M)\n";
  cout << "Fail Packets: " << fail_count << "(" << (double)fail_count / MPACKET
       << "M)\n";
  cout << "Elapsed Time: " << time_diff << "s\n";
  cout << "Performance : " << perf_kbps / KBYTE << "MBps/" << perf_kpps
       << "Kpps\n";
  cout << "--------------------------------------------------\n";
}

void Report::fail() noexcept { fail_count++; }

size_t Report::get_sent_count() noexcept { return sent_count; }

// vim: et:ts=2:sw=2
