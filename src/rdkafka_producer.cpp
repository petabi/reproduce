#include <iostream>

#include "rdkafka_producer.h"

using namespace std;

void RdDeliveryReportCb::dr_cb(RdKafka::Message& message)
{
  cout << "Message delivery for (" << message.len()
       << " bytes): " << message.errstr() << "\n";

  if (message.key()) {
    cout << "Key: " << *(message.key()) << ";"
         << "\n";
  }
}

void RdEventCb::event_cb(RdKafka::Event& event)
{
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " << event.str()
         << "\n";
    if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN) {
      // run = false;
    }
    break;

  case RdKafka::Event::EVENT_STATS:
    cerr << "\"STATS\": " << event.str() << "\n";
    break;

  case RdKafka::Event::EVENT_LOG:
    fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(), event.fac().c_str(),
            event.str().c_str());
    break;

  default:
    cerr << "EVENT " << event.type() << " (" << RdKafka::err2str(event.err())
         << "): " << event.str() << "\n";
    break;
  }
}

Rdkafka_producer::Rdkafka_producer(const string& brokers, const string& _topic)
{
  if (brokers.empty() || _topic.empty()) {
    throw runtime_error("Invalid Constructor Parameter!");
  }

  // Create configuration objects
  conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

  string errstr;

  // Set configuration properties
  conf->set("metadata.broker.list", brokers, errstr);
  conf->set("event_cb", &rd_event_cb, errstr);

#if 0
  // FIXME: when debug option is enabled, enable this code
  // show kafka producer config
  int pass;
  for (pass = 0; pass < 2; pass++) {
    list<string>* dump;
    if (pass == 0) {
      dump = conf->dump();
      cout << "# Global config"
           << "\n";
    } else {
      dump = tconf->dump();
      cout << "# Topic config"
           << "\n";
    }
    for (list<string>::iterator it = dump->begin(); it != dump->end();) {
      cout << *it << " = ";
      it++;
      cout << *it << "\n";
      it++;
    }
    cout << "\n";
  }
#endif

  // Set delivery report callback
  conf->set("dr_cb", &rd_dr_cb, errstr);
  producer = RdKafka::Producer::create(conf, errstr);
  if (!producer) {
    throw runtime_error("Failed to create producer: " + errstr);
  }

  // Create topic handle.
  topic = RdKafka::Topic::create(producer, _topic, tconf, errstr);
  if (!topic) {
    throw runtime_error("Failed to create topic: " + errstr);
  }
}

bool Rdkafka_producer::produce(const string& message)
{
  // Produce message
  RdKafka::ErrorCode resp = producer->produce(
      topic, RdKafka::Topic::PARTITION_UA,
      RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      const_cast<char*>(message.c_str()), message.size(), NULL, NULL);
  if (resp != RdKafka::ERR_NO_ERROR) {
    cerr << "% Produce failed: " << RdKafka::err2str(resp) << "\n";
  } else {
    cerr << "% Produced message (" << message.size() << " bytes)"
         << "\n";
  }
  producer->poll(0);

  while (producer->outq_len() > 0) {
    cerr << "Waiting for " << producer->outq_len() << "\n";
    producer->poll(1000);
  }

  return true;
}

Rdkafka_producer::~Rdkafka_producer()
{
  RdKafka::wait_destroyed(1000);

  delete topic;
  delete producer;
  delete conf;
  delete tconf;
}

// vim: et:ts=2:sw=2
