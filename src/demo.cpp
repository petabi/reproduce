#include <cstring>
#include <iostream>
#include <unistd.h>

#include "header2log.h"
#include "logConv.h"
#include "options.h"
#include "rdkafka_producer.h"

using namespace std;

static constexpr size_t MESSAGE_SIZE = 1024;
static const char* program_name = "header2log";
static const char* sample_data =
    "1531980827 Ethernet2 a4:7b:2c:1f:eb:61 40:61:86:82:e9:26 IP 4 5 0 10240 "
    "58477 64 127 47112 59.7.91.107 123.141.115.52 ip_opt TCP 62555 80 "
    "86734452 2522990538 20 A 16425 7168 0";

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

enum inputType { PCAP, PCAPNG, UNKNOWN, ERROR };
inputType check_input_type(const string& filepath)
{
  const unsigned char mn_pcap_little_micro[4] = {0xd4, 0xc3, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_micro[4] = {0xa1, 0xb2, 0xc3, 0xd4};
  const unsigned char mn_pcap_little_nano[4] = {0x4d, 0x3c, 0xb2, 0xa1};
  const unsigned char mn_pcap_big_nano[4] = {0xa1, 0xb2, 0x3c, 0x4d};
  const unsigned char mn_pcapng_little[4] = {0x4D, 0x3C, 0x2B, 0x1A};
  const unsigned char mn_pcapng_big[4] = {0x1A, 0x2B, 0x3C, 0x4D};

  ifstream input(filepath, ios::binary);
  if (!input.is_open()) {
    return inputType::ERROR;
  }

  input.seekg(0, ios::beg);
  unsigned char magic[4] = {0};
  input.read((char*)magic, sizeof(magic));

  if (memcmp(magic, mn_pcap_little_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_micro, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_little_nano, sizeof(magic)) == 0 ||
      memcmp(magic, mn_pcap_big_nano, sizeof(magic)) == 0) {
    return inputType::PCAP;
  } else if (memcmp(magic, mn_pcapng_little, sizeof(magic)) == 0 ||
             memcmp(magic, mn_pcapng_big, sizeof(magic)) == 0) {
    return inputType::PCAPNG;
  }
  return inputType::UNKNOWN;
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
    Options opt(conf);

    opt.show_options();
    opt.dprint(F, "start");
    opt.start_evaluation();

    unique_ptr<Conv> cp = nullptr;
    unique_ptr<RdkafkaProducer> rpp = nullptr;
    char message[MESSAGE_SIZE];
    int length = 0;

    if (conf.mode_parse) {
      strcpy(message, sample_data);
      length = strlen(message);
      opt.dprint(F, "message=%s (%d)", message, length);
    } else {
      inputType input_type = check_input_type(conf.input);
      if (input_type == inputType::PCAP) {
        cp = make_unique<Pcap>(conf.input);
        opt.dprint(F, "input type=Pcap", message, length);
      } else if (input_type == inputType::UNKNOWN) {
        cp = make_unique<LogConv>(conf.input);
        opt.dprint(F, "input type=Log", message, length);
      } else {
        throw runtime_error("Specify the appropriate input (See help)");
      }
    }
    if (conf.count_skip) {
      if (!cp->skip(conf.count_skip)) {
        opt.dprint(F, "failed to skip(%d)", conf.count_skip);
      }
    }
    if (!conf.mode_kafka) {
      rpp = make_unique<RdkafkaProducer>(opt);
    }

    while (true) {
      if (!conf.mode_parse) {
        length = cp->get_next_stream(message, MESSAGE_SIZE);
        if (length > 0) {
          // do nothing
        } else if (length == RESULT_FAIL) {
          opt.increase_fail();
          continue;
        } else if (length == RESULT_NO_MORE) {
          break;
        } else {
          // can't get here
        }
      }
      if (!conf.mode_kafka) {
        rpp->produce(string(message));
      }
      opt.process_evaluation(length);
      opt.mprint("%s", message);
      opt.fprint(message);
      if (opt.check_count()) {
        break;
      }
    }
    opt.report_evaluation();
    opt.dprint(F, "end");
  } catch (exception const& e) {
    cerr << "Exception: " << e.what() << '\n';
  }

  return 0;
}

// vim:et:ts=2:sw=2
