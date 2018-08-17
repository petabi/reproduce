#include <stdarg.h>
#include <sys/stat.h>

#include "options.h"

using namespace std;

static constexpr double KBYTE = 1024.0;
static constexpr double MBYTE = KBYTE * KBYTE;
static constexpr double KPACKET = 1000.0;
static constexpr double MPACKET = KPACKET * KPACKET;

// default config
static const char* default_broker = "localhost:9092";
static const char* default_topic = "pcap";
static constexpr size_t default_count_queue = 10000;
static constexpr size_t sample_count = 1000000;

Options::Options(const Config& _conf)
    : conf(_conf), sent_byte(0), sent_packet(0), fail_packet(0), perf_kbps(0),
      perf_kpps(0), time_start(0), time_now(0), time_diff(0),
      input_type(InputType::None)

{
  // input is madatory when mode_parse is not set
  if (conf.input.empty() && !conf.mode_parse) {
    throw runtime_error("Must specify input (See help)");
  }

  // set default value
  if (conf.broker.empty()) {
    conf.broker = default_broker;
  }
  if (conf.topic.empty()) {
    conf.topic = default_topic;
  }
  if (conf.mode_parse && conf.count_send == 0) {
    conf.count_send = sample_count;
  }
  if (conf.count_queue == 0) {
    conf.count_queue = default_count_queue;
  }

  if (!conf.output.empty()) {
    if (!open_output_file()) {
      throw runtime_error("Failed to open output file: " + conf.output);
    }
  }
}

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
  input_type = other.input_type;
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
    input_type = other.input_type;
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

void Options::show_options() noexcept
{
  dprint(F, "mode_debug=%d", conf.mode_debug);
  dprint(F, "mode_evel=%d", conf.mode_eval);
  dprint(F, "mode_kafka=%d", conf.mode_kafka);
  dprint(F, "mode_parse=%d", conf.mode_parse);
  dprint(F, "count_send=%lu", conf.count_send);
  dprint(F, "count_skip=%lu", conf.count_skip);
  dprint(F, "count_queue=%u", conf.count_queue);
  dprint(F, "input=%s", conf.input.c_str());
  dprint(F, "input_type=%d", input_type);
  dprint(F, "output=%s", conf.output.c_str());
  dprint(F, "filter=%s", conf.filter.c_str());
  dprint(F, "broker=%s", conf.broker.c_str());
  dprint(F, "topic=%s", conf.topic.c_str());
}

void Options::dprint(const char* name, const char* fmt, ...) noexcept
{
  if (!conf.mode_debug) {
    return;
  }

  va_list args;

  fprintf(stdout, "[DEBUG] %s: ", name);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

void Options::eprint(const char* name, const char* fmt, ...) noexcept
{
  va_list args;

  fprintf(stdout, "[ERROR] %s: ", name);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

void Options::mprint(const char* fmt, ...) noexcept
{
  if (!conf.mode_debug) {
    return;
  }

  if (conf.mode_eval) {
    fprintf(stdout, "[%lu/%.1f/%lu/%.1f] ", sent_byte, perf_kbps, sent_packet,
            perf_kpps);
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

void Options::fprint(const char* message) noexcept
{
  if (conf.output.empty()) {
    return;
  }

  output_file << message << '\n';
}

bool Options::check_count() noexcept
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

void Options::process_evaluation(size_t length) noexcept
{
  sent_byte += length;
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

void Options::report_evaluation() noexcept
{
  if (!conf.mode_eval) {
    return;
  }

  struct stat st;

  fprintf(stdout, "--------------------------------------------------\n");
  if (stat(conf.input.c_str(), &st) != -1) {
    fprintf(stdout, "Input File  : %s (%.2fM)\n", conf.input.c_str(),
            (double)st.st_size / MBYTE);
  } else {
    fprintf(stdout, "Input File  : invalid\n");
  }
  fprintf(stdout, "Sent Bytes  : %lu(%.2fM)\n", sent_byte,
          (double)sent_byte / MBYTE);
  fprintf(stdout, "Sent Packets: %lu(%.2fM)\n", sent_packet,
          (double)sent_packet / MPACKET);
  fprintf(stdout, "Fail Packets: %lu(%.2fM)\n", fail_packet,
          (double)fail_packet / MPACKET);
  fprintf(stdout, "Elapsed Time: %.2fs\n", time_diff);
  fprintf(stdout, "Performance : %.2fMBps/%.2fKpps\n", perf_kbps / KBYTE,
          perf_kpps);
  fprintf(stdout, "--------------------------------------------------\n");
}

bool Options::open_output_file() noexcept
{
  output_file.open(conf.output, ios::out);
  if (!output_file.is_open()) {
    dprint(F, "Failed to write %s file", conf.output.c_str());
    return false;
  }

  return true;
}

// vim: et:ts=2:sw=2
