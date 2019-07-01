#include <iostream>
#include <unistd.h>

#include "librdkafka/rdkafkacpp.h"

#include "config.h"
#include "version.h"

using namespace std;

static constexpr size_t default_queue_period = 3;
static constexpr size_t default_datasource_id = 1;
static constexpr size_t default_queue_size = 900000;

void Config::help() const noexcept
{
  cout << program_name << "-" << program_version << " (librdkafka++-"
       << RdKafka::version_str() << ")\n";
  cout << "[USAGE] " << program_name << " [OPTIONS]\n";
  cout << "  -b: kafka broker list, [host1:port1,host2:port2,..]\n";
  cout << "  -c: send count\n";
  cout << "  -d: data source id\n";
  cout << "  -E: entropy ratio. The amount of maximum entropy allowed for a\n";
  cout << "      session (0.0 < entropy ratio <= 1.0). Default is 0.9.\n";
  cout << "      Only relevant for network packets.\n";
  cout << "  -e: evaluation mode. output statistical result of transmission\n"
          "      after job is terminated or stopped\n";
  cout << "  -f: tcpdump filter (when input is NIC)\n";
  cout << "      (reference : "
          "https://www.tcpdump.org/manpages/pcap-filter.7.html)\n";
  cout << "  -g: follow the growing input file\n";
  cout << "  -h: help\n";
  cout << "  -i: input [PCAPFILE/LOGFILE/DIR/NIC]\n";
  cout
      << "      If no 'i' option is given, input is internal sample data and\n";
  cout << "      If DIR is given, the g option is not supported.\n";
  cout << "  -k: kafka config file."
       << " (Ex: kafka.conf)\n"
       << "      it overrides default kafka config to user kafka config\n";
  cout << "  -m: match [Pattern FILE]\n";
  cout << "  -n: prefix of file name to send multiple files or directory\n";
  cout << "  -o: output [TEXTFILE/none]\n";
  cout << "      If no 'o' option is given, output is kafka\n";
  cout << "  -p: queue period time. how much time keep queued data."
       << " (default: " << default_queue_period << ")\n";
  cout << "  -q: queue size. how many bytes send once to kafka."
       << " (default: " << default_queue_size << ")\n";
  cout << "  -r: record [prefix of offset file]\n"
       << "      using this option will start the conversion after the "
          "previous \n"
       << "      conversion. The offset file name is managed by [input \n"
       << "      file]_[prefix]. Except when the input is a NIC.\n";
  cout << "  -s: skip count\n";
  cout
      << "  -t: kafka topic If the broker does not have a corresponding "
         "topic,\n"
      << "      the broker fails unless there is a setting that automatically\n"
      << "      creates the topic.\n";
}

bool Config::set(int argc, char** argv)
{
  int o;

  while ((o = getopt(argc, argv, "b:c:d:eE:f:ghi:k:m:n:o:p:q:r:s:t:")) != -1) {
    switch (o) {
    case 'b':
      kafka_broker = optarg;
      break;
    case 'c':
      count_send = strtoul(optarg, nullptr, 0);
      break;
    case 'd':
      datasource_id = static_cast<uint16_t>(stoul(optarg, nullptr, 0));
      break;
    case 'E':
      entropy_ratio = strtof(optarg, nullptr);
      if (entropy_ratio <= 0.0 || entropy_ratio > 1.0) {
        cerr << "Waring: Entropy ratio is out of range. Defaulting to 0.9.\n";
      }
      entropy_ratio = 0.9;
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
    case 'm':
      pattern_file = optarg;
      break;
    case 'n':
      file_prefix = optarg;
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
    case 'r':
      offset_prefix = optarg;
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

  set_default();
  show();
  check();

  return true;
}

void Config::set_default() noexcept
{
  Util::dprint(F, "set default config");
  if (queue_size <= 0 || queue_size > default_queue_size) {
    queue_size = default_queue_size;
  }
  if (queue_period == 0) {
    queue_period = default_queue_period;
  }
  if (datasource_id == 0) {
    datasource_id = default_datasource_id;
  }
}

void Config::check() const
{
  if (output.empty() && kafka_broker.empty()) {
    throw runtime_error("You must specify kafka broker list(-b)");
  }
  if (output.empty() && kafka_topic.empty()) {
    throw runtime_error("You must specify kafka topic(-t)");
  }
  if (input.empty() && output == "none") {
    throw runtime_error("You must specify input(-i) or output(-o) is not none");
  }

  // and so on...
}

void Config::show() const noexcept
{
  Util::dprint(F, "mode_eval=", mode_eval);
  Util::dprint(F, "mode_grow=", mode_grow);
  Util::dprint(F, "count_send=", count_send);
  Util::dprint(F, "count_skip=", count_skip);
  Util::dprint(F, "queue_size=", queue_size);
  Util::dprint(F, "queue_period=", queue_period);
  Util::dprint(F, "input=", input);
  Util::dprint(F, "output=", output);
  Util::dprint(F, "offset_prefix=", offset_prefix);
  Util::dprint(F, "packet_filter=", packet_filter);
  Util::dprint(F, "kafka_broker=", kafka_broker);
  Util::dprint(F, "kafka_topic=", kafka_topic);
  Util::dprint(F, "kafka_conf=", kafka_conf);
  Util::dprint(F, "file_prefix==", file_prefix);
  Util::dprint(F, "datasource_id=", datasource_id);
}

// vim: et:ts=2:sw=2
