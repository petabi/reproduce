#include <cstdarg>
#include <cstring>
#include <sys/stat.h>

#include "options.h"

using namespace std;

Options::Options(Config _conf) : conf(move(_conf)) {}

Options::Options(const Options& other)
{
  conf = other.conf;
  // do not use output when it is copied
  // (*this).open_output_file();
}

Options& Options::operator=(const Options& other)
{
  if (this != &other) {
    conf = other.conf;
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

#if 0
  if (conf.mode_eval) {
    cout << "[" << sent_byte << "/" << perf_kbps << "/" << sent_packet << "/"
         << perf_kpps << "] ";
  }
#endif

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

bool Options::check_count(const size_t sent_count) const noexcept
{
  if (conf.count_send == 0 || sent_count < conf.count_send) {
    return false;
  }

  return true;
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

// vim: et:ts=2:sw=2
