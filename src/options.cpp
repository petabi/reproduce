#include <fstream>
#include <stdarg.h>
#include <sys/stat.h>

#include "options.h"

static const double KBYTE = 1024.0;
static const double MBYTE = KBYTE * KBYTE;
static const double KPACKET = 1000.0;
static const double MPACKET = KPACKET * KPACKET;

Options::Options()
    : mode_debug(false), mode_eval(false), mode_kafka(false), mode_parse(false),
      count_send(0), count_skip(0), count_queue(0), sent_byte(0),
      sent_packet(0), perf_kbps(0), perf_kpps(0), time_start(0), time_now(0),
      time_diff(0), input_type(InputType::None)
{
}

Options::~Options() {}

void Options::show_options() noexcept
{
  dprint(F, "mode_debug=%d", mode_debug);
  dprint(F, "mode_evel=%d", mode_eval);
  dprint(F, "mode_kafka=%d", mode_kafka);
  dprint(F, "mode_parse=%d", mode_parse);
  dprint(F, "count_send=%lu", count_send);
  dprint(F, "count_skip=%lu", count_skip);
  dprint(F, "count_queue=%u", count_queue);
  dprint(F, "input=%s", input.c_str());
  dprint(F, "input_type=%d", input_type);
  dprint(F, "output=%s", output.c_str());
  dprint(F, "filter=%s", filter.c_str());
  dprint(F, "broker=%s", broker.c_str());
  dprint(F, "topic=%s", topic.c_str());
}

void Options::dprint(const char* name, const char* fmt, ...) noexcept
{
  if (!mode_debug) {
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
  if (!mode_debug) {
    return;
  }

  if (mode_eval) {
    fprintf(stdout, "[%lu/%.1f/%lu/%.1f] ", sent_byte, perf_kbps, sent_packet,
            perf_kpps);
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

void Options::fprint(std::ofstream& stream, const char* message) noexcept
{
  if (!stream.is_open())
    return;
  stream << message << '\n';
}

bool Options::check_count() noexcept
{
  if (count_send == 0 || sent_packet < count_send) {
    return false;
  }

  return true;
}

void Options::start_evaluation() noexcept
{
  if (!mode_eval) {
    return;
  }

  time_start = clock();
}

void Options::process_evaluation(size_t length) noexcept
{
  sent_byte += length;
  sent_packet++;

  if (!mode_eval) {
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
  if (!mode_eval) {
    return;
  }

  struct stat st;

  fprintf(stdout, "--------------------------------------------------\n");
  if (stat(input.c_str(), &st) != -1) {
    fprintf(stdout, "Input File  : %s (%.2fM)\n", input.c_str(),
            (double)st.st_size / MBYTE);
  } else {
    fprintf(stdout, "Input File  : invalid\n");
  }
  fprintf(stdout, "Sent Bytes  : %lu(%.2fM)\n", sent_byte,
          (double)sent_byte / MBYTE);
  fprintf(stdout, "Sent Packets: %lu(%.2fM)\n", sent_packet,
          (double)sent_packet / MPACKET);
  fprintf(stdout, "Elapsed Time: %.2fs\n", time_diff);
  fprintf(stdout, "Performance : %.2fMBps/%.2fKpps\n", perf_kbps / KBYTE,
          perf_kpps);
  fprintf(stdout, "--------------------------------------------------\n");
}
// vim: et:ts=2:sw=2
