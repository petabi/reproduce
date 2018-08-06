#include <stdarg.h>

#include "options.h"

const double KBYTE = 1024.0;
const double MBYTE = KBYTE * KBYTE;
const double KPACKET = 1000.0;
const double MPACKET = KPACKET * KPACKET;

Options::Options()
    : debug(false), eval(false), kafka(false), byte(0), packet(0), kbps(0),
      kpps(0), time_start(0), time_now(0), time_diff(0)
{
}

Options::~Options() {}

void Options::show_options()
{
  dprint(F, "debug=%d", debug);
  dprint(F, "evel=%d", eval);
  dprint(F, "kafka=%d", kafka);
  dprint(F, "filter=%s", filter.c_str());
  dprint(F, "input=%s", input.c_str());
  dprint(F, "output=%s", output.c_str());
}

void Options::dprint(const char* name, const char* fmt, ...)
{
  if (!debug)
    return;

  va_list args;

  fprintf(stdout, "[DEBUG] %s: ", name);
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

void Options::mprint(const char* fmt, ...)
{
  if (!debug)
    return;

  if (eval) {
    fprintf(stdout, "[%lu/%.1f/%lu/%.1f] ", byte, kbps, packet, kpps);
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

void Options::start_evaluation()
{
  if (!eval)
    return;

  time_start = clock();
}

void Options::process_evaluation(size_t length)
{
  if (!eval)
    return;

  time_now = clock();
  time_diff = (double)(time_now - time_start) / CLOCKS_PER_SEC;

  byte += length;
  packet++;

  if (time_diff) {
    kbps = (double)byte / KBYTE / time_diff;
    kpps = (double)packet / KPACKET / time_diff;
  }
}

void Options::report_evaluation()
{
  if (!eval)
    return;

  fprintf(stdout, "--------------------------------------------------\n");
  fprintf(stdout, "Sent Bytes  : %lu(%.2fM) (%.2f MBps)\n", byte,
          (double)byte / MBYTE, kbps / KBYTE);
  fprintf(stdout, "Sent Packets: %lu(%.2fM) (%.2f Kpps)\n", packet,
          (double)packet / MPACKET, kpps);
  fprintf(stdout, "Elapsed Time: %.1f Sec.\n", time_diff);
  fprintf(stdout, "--------------------------------------------------\n");
}

// vim: et:ts=2:sw=2
