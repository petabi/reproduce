#include <iostream>
#include <unistd.h>

#include "librdkafka/rdkafkacpp.h"

#include "config.h"
#include "version.h"

using namespace std;

static constexpr char KAFKA_BROKER[] = "localhost:9092";
static constexpr char KAFKA_TOPIC[] = "pcap";
static constexpr size_t QUEUE_PERIOD = 3;
static constexpr size_t QUEUE_SIZE = 900000;

void Config::help() const noexcept
{
  cout << PROGRAM_NAME << "-" << PROGRAM_VERSION << " (librdkafka++-"
       << RdKafka::version_str() << ")\n";
  cout << "[USAGE] " << PROGRAM_NAME << " [OPTIONS]\n";
  cout << "  -b: kafka broker list"
       << " (default: " << KAFKA_BROKER << ")\n";
  cout << "  -c: send count\n";
  cout << "  -d: debug mode. print debug messages\n";
  cout << "  -e: evaluation mode. report statistics\n";
  cout << "  -f: tcpdump filter (when input is NIC)\n";
  cout << "  -g: follow the growing input file\n";
  cout << "  -h: help\n";
  cout << "  -i: input [PCAPFILE/LOGFILE/NIC]\n";
  cout << "      If no 'i' option is given, input is internal sample data\n";
  cout << "  -k: kafka config file"
       << " (Ex: kafka.conf)\n"
       << "      it overrides default kafka config to user kafka config\n";
  cout << "  -o: output [TEXTFILE/none]\n";
  cout << "      If no 'o' option is given, output is kafka\n";
  cout << "  -p: queue period time. how much time keep queued data."
       << " (default: " << QUEUE_PERIOD << ")\n";
  cout << "  -q: queue size. how many bytes send once to kafka."
       << " (default: " << QUEUE_SIZE << ")\n";
  cout << "  -s: skip count\n";
  cout << "  -t: kafka topic"
       << " (default: " << KAFKA_TOPIC << ")\n";
}

bool Config::set(int argc, char** argv)
{
  int o;
  while ((o = getopt(argc, argv, "b:c:def:ghi:k:o:p:q:s:t:")) != -1) {
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
    case 'p':
      queue_period = strtoul(optarg, nullptr, 0);
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
    kafka_broker = KAFKA_BROKER;
  }

  if (kafka_topic.empty()) {
    kafka_topic = KAFKA_TOPIC;
  }
  if (queue_size <= 0 || queue_size > QUEUE_SIZE) {
    queue_size = QUEUE_SIZE;
  }
  if (queue_period == 0) {
    queue_period = QUEUE_PERIOD;
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
  Util::dprint(F, "queue_period=", queue_period);
  Util::dprint(F, "input=", input);
  Util::dprint(F, "output=", output);
  Util::dprint(F, "packet_filter=", packet_filter);
  Util::dprint(F, "kafka_broker=", kafka_broker);
  Util::dprint(F, "kafka_topic=", kafka_topic);
  Util::dprint(F, "kafka_conf=", kafka_conf);
}

// vim: et:ts=2:sw=2
