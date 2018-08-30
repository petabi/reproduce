#include <cstring>
#include <iostream>
#include <unistd.h>

#include "controller.h"
#include "converter.h"
#include "options.h"
#include "producer.h"

using namespace std;

static const char* program_name = "packetproducer";
extern const char* default_broker;
extern const char* default_topic;

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
  cout << "  -i: input file(pcap/log) or nic\n";
  cout << "  -k: do not send data to kafka\n";
  cout << "  -o: output file\n";
  cout << "  -p: do not parse packet"
       << " (send hardcoded sample data. with -c option)\n";
  cout << "  -q: queue byte (how many bytes send once)\n";
  cout << "  -s: skip packet count\n";
  cout << "  -t: kafka topic"
       << " (default: " << default_topic << ")\n";
}

int main(int argc, char** argv)
{
  int o;
  Config conf;

  while ((o = getopt(argc, argv, "b:c:defhi:ko:pq:s:t:")) != -1) {
    switch (o) {
    case 'b':
      conf.broker = optarg;
      break;
    case 'c':
      conf.count_send = strtoul(optarg, nullptr, 0);
      break;
    case 'd':
      conf.mode_debug = true;
      break;
    case 'e':
      conf.mode_eval = true;
      break;
    case 'f':
      // FIXME: not implemented yet
      conf.filter = optarg;
      break;
    case 'i':
      // FIXME: not support log file and nic yet
      conf.input = optarg;
      break;
    case 'k':
      conf.mode_kafka = true;
      break;
    case 'o':
      conf.output = optarg;
      break;
    case 'p':
      conf.mode_parse = true;
      break;
    case 'q':
      conf.queue_size = strtoul(optarg, nullptr, 0);
      break;
    case 's':
      conf.count_skip = strtoul(optarg, nullptr, 0);
      break;
    case 't':
      conf.topic = optarg;
      break;
    case 'h':
    default:
      help();
      exit(0);
    }
  }

  try {
    Controller ctrl(conf);
    ctrl.run();
  } catch (exception const& e) {
    cerr << "[EXCEPTION] " << e.what() << "\n";
  }

  return 0;
}

// vim:et:ts=2:sw=2
