#include <cstdarg>
#include <cstring>
#include <sys/stat.h>

#include "options.h"

using namespace std;

static constexpr double KBYTE = 1024.0;
static constexpr double MBYTE = KBYTE * KBYTE;
static constexpr double KPACKET = 1000.0;
static constexpr double MPACKET = KPACKET * KPACKET;

Options::Options(Config _conf) : conf(move(_conf)) {}

Options::Options(const Options& other)
{
  conf = other.conf;
  sent_byte = other.sent_byte;
  sent_packet = other.sent_packet;
  perf_kbps = other.perf_kbps;
  perf_kpps = other.perf_kpps;
  time_start = other.time_start;
  time_now = other.time_now;
  time_diff = other.time_diff;
  // do not use output when it is copied
  // (*this).open_output_file();
}

Options& Options::operator=(const Options& other)
{
  if (this != &other) {
    conf = other.conf;
    sent_byte = other.sent_byte;
    sent_packet = other.sent_packet;
    perf_kbps = other.perf_kbps;
    perf_kpps = other.perf_kpps;
    time_start = other.time_start;
    time_now = other.time_now;
    time_diff = other.time_diff;
    // do not use output when it is assigned
    // (*this).open_output_file();
  }

  return *this;
}

Options::~Options()
{
  if (output_file.is_open()) {
    output_file.close();
  }
}

void Options::mprint(const char* message) const noexcept
{
  if (!conf.mode_debug) {
    return;
  }

  if (conf.mode_eval) {
    cout << "[" << sent_byte << "/" << perf_kbps << "/" << sent_packet << "/"
         << perf_kpps << "] ";
  }

  cout << message << "\n";
}

void Options::show_options() const noexcept
{
  dprint(F, "mode_debug=", conf.mode_debug);
  dprint(F, "mode_eval=", conf.mode_eval);
  dprint(F, "count_send=", conf.count_send);
  dprint(F, "count_skip=", conf.count_skip);
  dprint(F, "queue_size=", conf.queue_size);
  dprint(F, "input=", conf.input);
  dprint(F, "output=", conf.output);
  dprint(F, "filter=", conf.filter);
  dprint(F, "broker=", conf.broker);
  dprint(F, "topic=", conf.topic);
}

bool Options::check_count() const noexcept
{
  if (conf.count_send == 0 || sent_packet < conf.count_send) {
    return false;
  }

  return true;
}

void Options::start_evaluation() noexcept
{
  if (!conf.mode_eval) {
    return;
  }

  time_start = clock();
}

void Options::process_evaluation(int length) noexcept
{
  if (length > 0) {
    sent_byte += length;
  }
  sent_packet++;

  if (!conf.mode_eval) {
    return;
  }

  time_now = clock();
  time_diff = (double)(time_now - time_start) / CLOCKS_PER_SEC;

  if (time_diff) {
    perf_kbps = (double)sent_byte / KBYTE / time_diff;
    perf_kpps = (double)sent_packet / KPACKET / time_diff;
  }
}

void Options::report_evaluation() const noexcept
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
         << ")\n";
  } else {
    cout << "Input File  : invalid\n";
  }
  cout << "Sent Bytes  : " << sent_byte << "(" << (double)sent_byte / MBYTE
       << "M)\n";
  cout << "Sent Packets: " << sent_packet << "("
       << (double)sent_packet / MPACKET << "M)\n";
  cout << "Fail Packets: " << fail_packet << "("
       << (double)fail_packet / MPACKET << "M)\n";
  cout << "Elapsed Time: " << time_diff << "s\n";
  cout << "Performance : " << perf_kbps / KBYTE << "MBps/" << perf_kpps
       << "Kpps\n";
  cout << "--------------------------------------------------\n";
}

bool Options::open_output_file() noexcept
{
  output_file.open(conf.output, ios::out);
  if (!output_file.is_open()) {
    eprint(F, "Failed to write output file: ", conf.output);
    return false;
  }

  return true;
}

void Options::increase_fail() noexcept { fail_packet++; }

// vim: et:ts=2:sw=2
