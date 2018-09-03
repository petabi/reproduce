#include <iostream>
#include <unistd.h>

#include "config.h"

using namespace std;

static const char* default_broker = "localhost:9092";
static const char* default_topic = "pcap";
static constexpr size_t default_queue_size = 900000;
static constexpr size_t default_count = 1000000;

void Config::help() const noexcept
{
  cout << PROGRAM_NAME << "-" << PROGRAM_VERSION << "\n";
  cout << "[USAGE] " << PROGRAM_NAME << " [OPTIONS]\n";
  cout << "  -b: kafka broker"
       << " (default: " << default_broker << ")\n";
  cout << "  -c: send count\n";
  cout << "  -d: debug mode. print debug messages\n";
  cout << "  -e: evaluation mode. report statistics\n";
  cout << "  -f: tcpdump filter (when input is PCAP or NIC)\n";
  cout << "  -h: help\n";
  cout << "  -i: input [PCAPFILE/LOGFILE/NIC/none(no specification)])\n";
  cout << "  -o: output [kafka(no specification)/TEXTFILE/none] (default: "
          "kafka)\n";
  cout << "  -q: queue size in byte. how many bytes send once\n";
  cout << "  -s: skip count\n";
  cout << "  -t: kafka topic"
       << " (default: " << default_topic << ")\n";
}

bool Config::set(int argc, char** argv)
{
  int o;
  while ((o = getopt(argc, argv, "b:c:defhi:o:q:s:t:")) != -1) {
    switch (o) {
    case 'b':
      broker = optarg;
      break;
    case 'c':
      count_send = strtoul(optarg, nullptr, 0);
      break;
    case 'd':
      mode_debug = true;
      break;
    case 'e':
      mode_eval = true;
      break;
    case 'f':
      // TODO: not implemented yet
      filter = optarg;
      break;
    case 'h':
      help();
      return false;
    case 'i':
      // TODO: nic
      input = optarg;
      break;
    case 'o':
      output = optarg;
      break;
    case 'q':
      queue_size = strtoul(optarg, nullptr, 0);
      break;
    case 's':
      count_skip = strtoul(optarg, nullptr, 0);
      break;
    case 't':
      topic = optarg;
      break;
    default:
      break;
    }
  }

  set_default();
  show();
  check();

  return true;
}

void Config::set_default() noexcept
{
  Util::dprint(F, "set default config");

  if (broker.empty()) {
    broker = default_broker;
  }

  if (topic.empty()) {
    topic = default_topic;
  }

  if (queue_size == 0) {
    queue_size = default_queue_size;
  }
}

void Config::check() const
{
  Util::dprint(F, "check config");

  // TODO: add config restriction
#if 0
  if (input.empty() && output.empty()) {
    throw runtime_error("You must specify input(-i) or output(-o)");
  }
#endif
}

void Config::show() const noexcept
{
  Util::dprint(F, "mode_debug=", mode_debug);
  Util::dprint(F, "mode_eval=", mode_eval);
  Util::dprint(F, "count_send=", count_send);
  Util::dprint(F, "count_skip=", count_skip);
  Util::dprint(F, "queue_size=", queue_size);
  Util::dprint(F, "input=", input);
  Util::dprint(F, "output=", output);
  Util::dprint(F, "filter=", filter);
  Util::dprint(F, "broker=", broker);
  Util::dprint(F, "topic=", topic);
}

// vim: et:ts=2:sw=2
