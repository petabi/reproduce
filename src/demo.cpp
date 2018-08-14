#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fstream>

#include "header2log.h"
#include "options.h"
#include "rdkafka_producer.h"

using namespace std;

static const char* program_name = "header2log";
static const char* default_broker = "localhost:9092";
static const char* default_topic = "pcap";
static const size_t default_count_queue = 1000;
static const char* sample_data =
    "Thu Jul 19 06:13:51 2018 Ethernet2 40:61:86:82:e9:26 a4:7b:2c:1f:eb:61 IP "
    "4 5 0 5 9 0 8 15 183.111.246.132 59.7.91.240 TCP 443 22866 3705800231 "
    "115518086 20";
static const size_t sample_count = 1000000;

void help()
{
  cout << "[USAGE] " << program_name << " OPTIONS\n";
  cout << "  -b: kafka broker"
       << " (default: " << default_broker << ")\n";
  cout << "  -c: send packet count\n";
  cout << "  -d: debug mode (print debug messages)\n";
  cout << "  -e: evaluation mode (report statistics)\n";
  cout << "  -f: tcpdump filter\n";
  cout << "  -h: help\n";
  cout << "  -i: input pcapfile or nic\n";
  cout << "  -k: do not send data to kafka\n";
  cout << "  -o: output file\n";
  cout << "  -p: do not parse packet"
       << " (send hardcoded sample data. with -c option)\n";
  cout << "  -q: queue packet count (how many packet send once)\n";
  cout << "  -s: skip packet count\n";
  cout << "  -t: kafka topic"
       << " (default: " << default_topic << ")\n";
}

int main(int argc, char** argv)
{
  int o;
  Options opt;
  std::ofstream output_file;

  while ((o = getopt(argc, argv, "b:c:defhi:ko:pq:s:t:")) != -1) {
    switch (o) {
    case 'b':
      opt.broker = optarg;
      break;
    case 'c':
      opt.count_send = strtoul(optarg, nullptr, 0);
      break;
    case 'd':
      opt.mode_debug = true;
      break;
    case 'e':
      opt.mode_eval = true;
      break;
    case 'f':
      // FIXME: not implemented yet
      opt.filter = optarg;
      break;
    case 'i':
      // FIXME: not support nic yet
      opt.input = optarg;
      break;
    case 'k':
      opt.mode_kafka = true;
      break;
    case 'o':
      opt.output = optarg;
      output_file.open(opt.output, ios::out);
	  if (!output_file.is_open())
        opt.dprint(F, "exception: %s", "failed access output file");
      opt.mode_write_output = true;
      break;
    case 'p':
      opt.mode_parse = true;
      break;
    case 'q':
      opt.count_queue = strtoul(optarg, nullptr, 0);
      break;
    case 's':
      // FIXME: not implemented yet
      opt.count_skip = strtoul(optarg, nullptr, 0);
      break;
    case 't':
      opt.topic = optarg;
      break;
    case 'h':
    default:
      help();
      exit(0);
    }
  }

  // input is madatory when mode_parse is not set
  if (opt.input.empty() && !opt.mode_parse) {
    help();
    exit(0);
  }

  // set default value
  if (opt.broker.empty()) {
    opt.broker = default_broker;
  }
  if (opt.topic.empty()) {
    opt.topic = default_topic;
  }
  if (opt.mode_parse && opt.count_send == 0) {
    opt.count_send = sample_count;
  }
  if (opt.count_queue == 0) {
    opt.count_queue = default_count_queue;
  }

  opt.show_options();

  opt.dprint(F, "start");
  opt.start_evaluation();

  try {
    // FIXME: we need Pcap() default constructor (without input)
    Pcap pcap(opt.input);
    Rdkafka_producer rp(opt);
    char message[1024];

    /* FIXME: skip_bytes() --> skip_packets()
    if (opt.skip) {
      pcap.skip_bytes(opt.skip);
    }
    */

    if (opt.mode_parse) {
      strcpy(message, sample_data);
    }
    size_t stream_length = 0;
	int i =0;
    while (true) {
		i++;
      if (!opt.mode_parse) {
        stream_length = pcap.get_next_stream(message);
      }
      if (stream_length == 0) {
        break;
      }
      if (!opt.mode_kafka) {
        rp.produce(string(message));
      }
      opt.process_evaluation(stream_length);
      opt.mprint("%s", message);
      if (opt.mode_write_output)
        opt.fprint(output_file, message);
      if (opt.check_count()) {
        break;
      }
    }
  } catch (exception const& e) {
    opt.dprint(F, "exception: %s", e.what());
  }

  opt.report_evaluation();
  opt.dprint(F, "end");

  return 0;
}

// vim:et:ts=2:sw=2
