#include <iostream>
#include <unistd.h>

#include "header2log.h"
#include "options.h"
#include "rdkafka_producer.h"

using namespace std;

static const char* program_name = "header2log";
static const char* default_broker = "localhost:9092";
static const char* default_topic = "pcap";

void help()
{
  cout << "[USAGE] " << program_name << " OPTIONS" << '\n';
  cout << "  -b: kafka broker"
       << " (default: " << default_broker << ")" << '\n';
  cout << "  -c: packet count to send" << '\n';
  cout << "  -d: debug mode (print debug messages)" << '\n';
  cout << "  -e: evaluation mode (report statistics)" << '\n';
  cout << "  -f: tcpdump filter" << '\n';
  cout << "  -h: help" << '\n';
  cout << "  -i: input pcapfile or nic (mandatory)" << '\n';
  cout << "  -k: do not send to kafka" << '\n';
  cout << "  -o: output file" << '\n';
  cout << "  -s: skip count" << '\n';
  cout << "  -t: kafka topic"
       << " (default: " << default_topic << ")" << '\n';
}

int main(int argc, char** argv)
{
  int o;
  Options opt;

  while ((o = getopt(argc, argv, "b:c:defhi:ko:s:t:")) != -1) {
    switch (o) {
    case 'b':
      opt.broker = optarg;
      break;
    case 'c':
      opt.count = strtoul(optarg, nullptr, 0);
      break;
    case 'd':
      opt.debug = true;
      break;
    case 'e':
      opt.eval = true;
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
      opt.kafka = true;
      break;
    case 'o':
      // FIXME: not implemented yet
      opt.output = optarg;
      break;
    case 's':
      // FIXME: not implemented yet
      opt.skip = strtoul(optarg, nullptr, 0);
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

  if (opt.input.empty()) {
    help();
    exit(0);
  }

  if (opt.broker.empty()) {
    opt.broker = default_broker;
  }

  if (opt.topic.empty()) {
    opt.topic = default_topic;
  }

  opt.show_options();

  opt.dprint(F, "start");
  opt.start_evaluation();

  try {
    Pcap pcap(opt.input);
    Rdkafka_producer rp(opt.broker, opt.topic);
    string message;

    /* FIXME: skip_bytes() --> skip_packet()
    if (opt.skip) {
      pcap.skip_bytes(opt.skip);
    }
    */

    while (true) {
      message = pcap.get_next_stream();
      if (message.empty()) {
        break;
      }
      if (!opt.kafka) {
        rp.produce(message);
      }
      opt.process_evaluation(message.length());
      opt.mprint("%s", message.c_str());
      if (opt.check_count()) {
        break;
      }
    }
  } catch (exception const& e) {
    opt.dprint(F, "exception: %s", e.what());
  }

  opt.dprint(F, "end");
  opt.report_evaluation();

  return 0;
}

// vim:et:ts=2:sw=2
