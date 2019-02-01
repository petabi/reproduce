#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "config.h"

TEST(test_config, test_config_basic)
{
  bool mode_eval = true;
  bool mode_grow = true;
  size_t count_skip = 11;
  size_t q_size = 12;
  size_t q_period = 13;
  std::string input = "myin";
  std::string output = "myout";
  std::string offset_prefix = "myoffset";
  std::string packet_filter = "myfilter";
  std::string kafka_broker = "mybroker";
  std::string kafka_topic = "mytopic";
  std::string kafka_conf = "mykconf";
  size_t count_send = 14;

  Config conf;
  conf.mode_eval = mode_eval;
  conf.mode_grow = mode_grow;
  conf.count_skip = count_skip;
  conf.queue_size = q_size;
  conf.queue_period = q_period;
  conf.input = input;
  conf.output = output;
  conf.offset_prefix = offset_prefix;
  conf.packet_filter = packet_filter;
  conf.kafka_broker = kafka_broker;
  conf.kafka_topic = kafka_topic;
  conf.kafka_conf = kafka_conf;
  conf.count_send = count_send;

  Config conf2 = conf;
  EXPECT_TRUE(conf.mode_eval == conf2.mode_eval);
  EXPECT_TRUE(conf.mode_grow == conf2.mode_grow);
  EXPECT_TRUE(conf.count_skip == conf2.count_skip);
  EXPECT_TRUE(conf.queue_size == conf2.queue_size);
  EXPECT_TRUE(conf.queue_period == conf2.queue_period);
  EXPECT_TRUE(conf.input == conf2.input);
  EXPECT_TRUE(conf.output == conf2.output);
  EXPECT_TRUE(conf.offset_prefix == conf2.offset_prefix);
  EXPECT_TRUE(conf.packet_filter == conf2.packet_filter);
  EXPECT_TRUE(conf.kafka_broker == conf2.kafka_broker);
  EXPECT_TRUE(conf.kafka_topic == conf2.kafka_topic);
  EXPECT_TRUE(conf.kafka_conf == conf2.kafka_conf);
  EXPECT_TRUE(conf.count_send == conf2.count_send);

  Config conf3;
  std::vector<std::string> myoptions = {"reproduce",
                                        "-b",
                                        kafka_broker,
                                        "-c",
                                        std::to_string(count_send),
                                        "-e",
                                        "-f",
                                        packet_filter,
                                        "-g",
                                        "-i",
                                        input,
                                        "-k",
                                        kafka_conf,
                                        "-o",
                                        output,
                                        "-p",
                                        std::to_string(q_period),
                                        "-q",
                                        std::to_string(q_size),
                                        "-r",
                                        offset_prefix,
                                        "-s",
                                        std::to_string(count_skip),
                                        "-t",
                                        kafka_topic};
  std::vector<char*> myargs;
  for (const auto& arg : myoptions) {
    myargs.push_back(const_cast<char*>(arg.data()));
  }
  myargs.push_back(nullptr);
  conf3.set(static_cast<int>(myargs.size() - 1), myargs.data());
  EXPECT_TRUE(conf.mode_eval == conf3.mode_eval);
  EXPECT_TRUE(conf.mode_grow == conf3.mode_grow);
  EXPECT_TRUE(conf.count_skip == conf3.count_skip);
  EXPECT_TRUE(conf.queue_size == conf3.queue_size);
  EXPECT_TRUE(conf.queue_period == conf3.queue_period);
  EXPECT_TRUE(conf.input == conf3.input);
  EXPECT_TRUE(conf.output == conf3.output);
  EXPECT_TRUE(conf.offset_prefix == conf3.offset_prefix);
  EXPECT_TRUE(conf.packet_filter == conf3.packet_filter);
  EXPECT_TRUE(conf.kafka_broker == conf3.kafka_broker);
  EXPECT_TRUE(conf.kafka_topic == conf3.kafka_topic);
  EXPECT_TRUE(conf.kafka_conf == conf3.kafka_conf);
  EXPECT_TRUE(conf.count_send == conf3.count_send);
}
