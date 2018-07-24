#include "header2log.h"
#include <iostream>
using namespace std;

const char* broker = "10.90.180.54:9092";
const char* topic = "pcap";

int main(int argc, char** argv)
{
  bool end = false;
  if (argc != 2)
    return -1;

  Pcap pcap;
  if (!pcap.open_pcap(argv[1])) {
    cout << "pcap file is wrong..\n";
    return -1;
  }
  // skip by bytes
  // pcap.skip_bytes(1000);
  while (!end) {
    if (pcap.get_next_stream()) {
      pcap.produce_to_rdkafka(broker, topic);
      // print log stream per packet
      // pcap.print_log_stream();
    } else
      end = true;
  }
}
