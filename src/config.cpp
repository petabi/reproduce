#include <iostream>
#include <unistd.h>

#include "librdkafka/rdkafkacpp.h"

#include "config.h"
#include "version.h"

using namespace std;

static constexpr char default_kafka_broker[] = "localhost:9092";
static constexpr char default_kafka_topic[] = "pcap";

void Config::help() const noexcept
{
  cout << PROGRAM_NAME << "-" << PROGRAM_VERSION << " (librdkafka++-"
       << RdKafka::version_str() << ")\n";
  cout << "[USAGE] " << PROGRAM_NAME << " [OPTIONS]\n";
  cout << "  -b: kafka broker list"
       << " (default: " << default_kafka_broker << ")\n";
  cout << "  -c: send count\n";
  cout << "  -d: debug mode. print debug messages\n";
  cout << "  -e: evaluation mode. report statistics\n";
  cout << "  -f: tcpdump filter (when input is PCAP or NIC)\n";
  cout << "  -g: follow the growing input file\n";
  cout << "  -h: help\n";
  cout << "  -i: input [PCAPFILE/LOGFILE/NIC]\n";
  cout << "      If no 'i' option is given, input is internal sample data\n";
  cout << "  -k: kafka config file"
       << " (Ex: kafka.conf)\n"
       << "      it overrides default kafka config to user kafka config\n";
  cout << "  -o: output [TEXTFILE/none]\n";
  cout << "      If no 'o' option is given, output is kafka\n";
  cout << "  -q: queue size. how many bytes send once to kafka\n"
       << "      (default: auto adjustment in proportion to bandwidth,\n"
       << "       min/max: " << QUEUE_SIZE_MIN << "~" << QUEUE_SIZE_MAX
       << ")\n";
  cout << "  -s: skip count\n";
  cout << "  -t: kafka topic"
       << " (default: " << default_kafka_topic << ")\n";
}

bool Config::set(int argc, char** argv)
{
  int o;
  while ((o = getopt(argc, argv, "b:c:def:ghi:k:o:q:s:t:")) != -1) {
    switch (o) {
    case 'b':
      kafka_broker = optarg;
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
      packet_filter = optarg;
      break;
    case 'g':
      mode_grow = true;
      break;
    case 'h':
      help();
      return false;
    case 'i':
      input = optarg;
      break;
    case 'k':
      kafka_conf = optarg;
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
      kafka_topic = optarg;
      break;
    default:
      break;
    }
  }

  Util::set_debug(mode_debug);

  set_default();
  show();
  check();

  return true;
}

void Config::set_default() noexcept
{
  Util::dprint(F, "set default config");

  if (kafka_broker.empty()) {
    kafka_broker = default_kafka_broker;
  }

  if (kafka_topic.empty()) {
    kafka_topic = default_kafka_topic;
  }

  if (queue_size < QUEUE_SIZE_MIN) {
    if (mode_grow) {
      queue_auto = true;
      queue_size = QUEUE_SIZE_MIN;
    } else {
      queue_size = QUEUE_SIZE_MAX;
    }
  } else {
    queue_defined = true;
  }

  if (queue_size < QUEUE_SIZE_MIN) {
    queue_size = QUEUE_SIZE_MIN;
  } else if (queue_size > QUEUE_SIZE_MAX) {
    queue_size = QUEUE_SIZE_MAX;
  }
}

void Config::check() const
{
  Util::dprint(F, "check config");

  if (input.empty() && output == "none") {
    throw runtime_error("You must specify input(-i) or output(-o) is not none");
  }

  // and so on...
}

void Config::show() const noexcept
{
  Util::dprint(F, "mode_debug=", mode_debug);
  Util::dprint(F, "mode_eval=", mode_eval);
  Util::dprint(F, "mode_grow=", mode_grow);
  Util::dprint(F, "count_send=", count_send);
  Util::dprint(F, "count_skip=", count_skip);
  Util::dprint(F, "queue_size=", queue_size);
  Util::dprint(F, "input=", input);
  Util::dprint(F, "output=", output);
  Util::dprint(F, "packet_filter=", packet_filter);
  Util::dprint(F, "kafka_broker=", kafka_broker);
  Util::dprint(F, "kafka_topic=", kafka_topic);
  Util::dprint(F, "kafka_conf=", kafka_conf);
}

// vim: et:ts=2:sw=2
