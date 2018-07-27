#include <iostream>

#include "header2log.h"
#include "rdkafka_producer.h"

using namespace std;

const char* broker = "10.90.180.54:9092";
const char* topic = "pcap";

int main(int argc, char** argv)
{
  try {
    if (argc != 2)
      return -1;
    string pcap_path = argv[1];
    Pcap pcap(pcap_path);
    Rdkafka_producer rp(broker, topic);
    string message;
    bool end = false;

    // skip by bytes
    // pcap.skip_bytes(1000);

    while (!end) {
      message = pcap.get_next_stream();
      if (rp.produce(message)) {
        cout << message;
      } else
        end = true;
    }
  } catch (exception const& e) {
    cout << "Exception: " << e.what();
  }
  return 0;
}
