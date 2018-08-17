#include <iostream>

#include "rdkafka_producer.h"

using namespace std;

void RdDeliveryReportCb::dr_cb(RdKafka::Message& message)
{
  opt.dprint(F, "message delivery(%lu bytes): %s", message.len(),
             message.errstr().c_str());

  if (message.key()) {
    opt.dprint(F, "key: %s", (*(message.key())).c_str());
  }
}

void RdEventCb::event_cb(RdKafka::Event& event)
{
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    opt.eprint(F, "%s: %s", RdKafka::err2str(event.err()).c_str(),
               event.str().c_str());
    if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN) {
      // FIXME: what do we do?
    }
    break;
  case RdKafka::Event::EVENT_STATS:
    opt.dprint(F, "STAT: %s", event.str().c_str());
    break;
  case RdKafka::Event::EVENT_LOG:
    opt.dprint(F, "LOG-%i-%s: %s", event.severity(), event.fac().c_str(),
               event.str().c_str());
    break;
  default:
    opt.eprint(F, "EVENT: %s: %s", RdKafka::err2str(event.err()).c_str(),
               event.str().c_str());
    break;
  }
}

RdkafkaProducer::RdkafkaProducer(const Options& _opt) : opt(_opt)
{
  if (opt.conf.broker.empty() || opt.conf.topic.empty()) {
    throw runtime_error("Invalid constructor parameter");
  }

  rd_dr_cb.opt = _opt;
  rd_event_cb.opt = _opt;

  // Create configuration objects
  conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  if (!conf) {
    throw runtime_error("Failed to create global configuration object");
  }
  tconf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  if (!tconf) {
    throw runtime_error("Failed to create topic configuration object");
  }

  string errstr;

  // Set configuration properties
  if (conf->set("metadata.broker.list", opt.conf.broker, errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: metadata.broker.list");
  }
  if (conf->set("event_cb", &rd_event_cb, errstr) != RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: event_cb");
  }
  if (conf->set("dr_cb", &rd_dr_cb, errstr) != RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: dr_cb");
  }

  // Set configuration properties: optional features
  if (conf->set("message.max.bytes", "100000000", errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: message.max.bytes");
  }
#if 0
  if (conf->set("message.copy.max.bytes", "1000000000", errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: message.copy.max.bytes");
  }
#endif
  if (conf->set("queue.buffering.max.messages", "10000000", errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: queue.buffering.max.messages");
  }
  if (conf->set("queue.buffering.max.kbytes", "2097151", errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: queue.buffering.max.kbytes");
  }
#if 0
  if (conf->set("compression.codec", "lz4", errstr) !=
      RdKafka::Conf::CONF_OK) {
    throw runtime_error("Failed to set config: compression.codec");
  }
#endif

  // show kafka producer config
  if (opt.conf.mode_debug) {
    int pass;
    for (pass = 0; pass < 2; pass++) {
      list<string>* dump;
      if (pass == 0) {
        dump = conf->dump();
        opt.dprint(F, "### Global Config");
      } else {
        dump = tconf->dump();
        opt.dprint(F, "### Topic Config");
      }
      // FIXME: iterator
      for (list<string>::iterator it = dump->begin(); it != dump->end();) {
        string key = *it++;
        string value = *it++;
        opt.dprint(F, "%s=%s", key.c_str(), value.c_str());
      }
    }
  }

  // Create producet handle
  producer.reset(RdKafka::Producer::create(conf.get(), errstr));
  if (!producer) {
    throw runtime_error("Failed to create producer: " + errstr);
  }

  // Create topic handle
  topic.reset(RdKafka::Topic::create(producer.get(), opt.conf.topic,
                                     tconf.get(), errstr));
  if (!topic) {
    throw runtime_error("Failed to create topic: " + errstr);
  }
}

void RdkafkaProducer::wait_queue(const int count) noexcept
{
  while (producer->outq_len() > count) {
    opt.dprint(F, "waiting for %d", producer->outq_len());

    // FIXME: test effects of timeout (original 1000)
    producer->poll(100);
  }
}

bool RdkafkaProducer::produce_core(const string& message) noexcept
{
  // Produce message
  RdKafka::ErrorCode resp = producer->produce(
      topic.get(), RdKafka::Topic::PARTITION_UA,
      RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
      const_cast<char*>(message.c_str()), message.size(), nullptr, nullptr);

  if (resp != RdKafka::ERR_NO_ERROR) {
    opt.eprint(F, "produce failed: %s", RdKafka::err2str(resp).c_str());
    return false;
  }

  opt.dprint(F, "produced message: %lu bytes", message.size());

  producer->poll(0);

  return true;
}

bool RdkafkaProducer::produce(const string& message) noexcept
{
  if (queue_data.length() + message.length() >= opt.conf.queue_size) {
    produce_core(queue_data);
    queue_data.clear();

    wait_queue(9999999);
  }
  queue_data += message + '\n';

  return true;
}

RdkafkaProducer::~RdkafkaProducer()
{
  opt.dprint(F, "last queued data: %u", queue_data.size());

  if (queue_data.size()) {
    produce_core(queue_data);
    queue_data.clear();
  }
  wait_queue(0);

  RdKafka::wait_destroyed(1000);
}

// vim: et:ts=2:sw=2
