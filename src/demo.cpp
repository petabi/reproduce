#include <iostream>
#include <unistd.h>

#include "header2log.h"
#include "options.h"
#include "rdkafka_producer.h"

using namespace std;

const char* program_name = "header2log";
const char* broker = "10.90.180.54:9092";
const char* topic = "pcap";

void help(void)
{
  cout << "[USAGE] " << program_name << " OPTIONS" << '\n';
  cout << "  -d: debug mode (print debug messages)" << '\n';
  cout << "  -e: evaluation mode (report statistics)" << '\n';
  cout << "  -f: tcpdump filter" << '\n';
  cout << "  -h: help" << '\n';
  cout << "  -i: input pcapfile or nic (mandatory)" << '\n';
  cout << "  -k: do not send to kafka" << '\n';
  cout << "  -o: output file" << '\n';
}

int main(int argc, char** argv)
{
  int o;
  Options opt;

  while ((o = getopt(argc, argv, "defhi:k")) != -1) {
    switch (o) {
    case 'd':
      opt.debug = true;
      break;
    case 'e':
      opt.eval = true;
      break;
    case 'f':
      // not implemented yet
      break;
    case 'i':
      opt.input = optarg;
      break;
    case 'k':
      opt.kafka = true;
      break;
    case 'o':
      opt.output = optarg;
      break;
    case 'h':
    default:
      help();
      exit(0);
    }
  }

  opt.show_options();

  if (opt.input.size() == 0) {
    help();
    exit(0);
  }

  opt.dprint(F, "start");
  opt.start_evaluation();

  try {
    Pcap pcap(opt.input);
    Rdkafka_producer rp(broker, topic);
    string message;
    bool end = false;

    // skip by bytes
    // pcap.skip_bytes(1000);

    while (!end) {
      message = pcap.get_next_stream();
      if (!message.empty()) {
        if (!opt.kafka)
          rp.produce(message);
        opt.process_evaluation(message.length());
        opt.mprint("%s", message.c_str());
      } else {
        end = true;
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
