#include <cstdio>
#include <iostream>

#include "rdkafka_producer.h"

class RdDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message& message)
  {
    std::cout << "Message delivery for (" << message.len()
              << " bytes): " << message.errstr() << std::endl;
    if (message.key())
      std::cout << "Key: " << *(message.key()) << ";" << std::endl;
  }
};

class RdEventCb : public RdKafka::EventCb {
public:
  void event_cb(RdKafka::Event& event)
  {
    switch (event.type()) {
    case RdKafka::Event::EVENT_ERROR:
      std::cerr << "ERROR (" << RdKafka::err2str(event.err())
                << "): " << event.str() << std::endl;
      if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
        // run = false;
        break;

    case RdKafka::Event::EVENT_STATS:
      std::cerr << "\"STATS\": " << event.str() << std::endl;
      break;

    case RdKafka::Event::EVENT_LOG:
      fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(), event.fac().c_str(),
              event.str().c_str());
      break;

    default:
      std::cerr << "EVENT " << event.type() << " ("
                << RdKafka::err2str(event.err()) << "): " << event.str()
                << std::endl;
      break;
    }
  }
};

Rdkafka_producer::Rdkafka_producer(const std::string& brokers,
                                   const std::string& _topic)
{

  if (brokers.empty() || _topic.empty())
    throw std::runtime_error("Invalid Constructor Parameter!");
  /*
   * Create configuration objects
   */
  conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

  /*
   * Set configuration properties
   */
  conf->set("metadata.broker.list", brokers, errstr);

  RdEventCb ex_event_cb;
  conf->set("event_cb", &ex_event_cb, errstr);

  RdDeliveryReportCb ex_dr_cb;

  /* Set delivery report callback */
  conf->set("dr_cb", &ex_dr_cb, errstr);
  producer = RdKafka::Producer::create(conf, errstr);
  if (!producer) {
    std::cerr << "Failed to create producer: " << errstr << std::endl;
    exit(1);
  }

  /*
   * Create topic handle.
   */
  topic = RdKafka::Topic::create(producer, _topic, tconf, errstr);
  if (!topic) {
    throw std::runtime_error(std::string("Failed to create topic: ") + errstr);
  }
}

Rdkafka_producer::Rdkafka_producer(Rdkafka_producer&& other) noexcept
{
  std::cout << "%% Flushing final messages..\n";
}

bool Rdkafka_producer::produce(const std::string& message)
{
  /*
   * Produce message
   */
  RdKafka::ErrorCode resp = producer->produce(
      topic, partition, RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      const_cast<char*>(message.c_str()), message.size(), NULL, NULL);
  if (resp != RdKafka::ERR_NO_ERROR)
    std::cerr << "% Produce failed: " << RdKafka::err2str(resp) << std::endl;
  else
    std::cerr << "% Produced message (" << message.size() << " bytes)"
              << std::endl;

  producer->poll(0);

  //
  while (producer->outq_len() > 0) {
    std::cerr << "Waiting for " << producer->outq_len() << std::endl;
    producer->poll(1000);
  }
  return true;
}

Rdkafka_producer::~Rdkafka_producer()
{
  RdKafka::wait_destroyed(1000);

  delete conf;
  delete tconf;
  delete producer;
  delete topic;
}
