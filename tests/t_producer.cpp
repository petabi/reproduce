#include <cstddef>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "config.h"
#include "producer.h"

TEST(test_producer, DISABLED_test_producer_kafka)
{
  Config* conf = config_default();
  config_set_queue_size(conf, 1);
  config_set_queue_period(conf, 1);
  config_set_input(conf, "Log");
  config_set_output(conf, "mytestfile.log");
  config_set_kafka_broker(conf, "[localhost:9001]");
  config_set_kafka_topic(conf, "test_topic");
  config_set_input_type(conf, InputType::Log);
  config_set_output_type(conf, OutputType::File);
  KafkaProducer kp(conf);
  size_t mb = kp.get_max_bytes();
  EXPECT_GT(mb, 0);
  // Need to test produce but avoid eternal wait.
  // Also, test causes an abort as is.
  config_free(conf);
}

TEST(test_producer, test_producer_file)
{
  Config* conf = config_default();
  config_set_queue_size(conf, 1);
  config_set_queue_period(conf, 1);
  config_set_input(conf, "Log");
  config_set_output(conf, "mytestfile.log");
  config_set_input_type(conf, InputType::Log);
  config_set_output_type(conf, OutputType::File);
  FileProducer fp(conf);
  size_t mb = fp.get_max_bytes();
  EXPECT_GT(mb, 0);
  std::string mymessage = "Here is a message to produce!";
  const char* c = mymessage.c_str();
  bool success = fp.produce(c, mymessage.size(), true);
  EXPECT_TRUE(success);
  std::ifstream myfile(config_output(conf));
  std::vector<char> mybuffer;
  // getline appends a \0 at the end.
  mybuffer.resize(mymessage.size() + 1, 0);
  myfile.getline(mybuffer.data(),
                 static_cast<std::streamsize>(mybuffer.size()));
  EXPECT_STREQ(mymessage.c_str(), mybuffer.data());
  std::remove(mymessage.c_str());
  config_free(conf);
}

TEST(test_producer, test_producer_null)
{
  NullProducer np;
  size_t mb = np.get_max_bytes();
  EXPECT_GT(mb, 0);
  std::string msg = "A message to lose!";
  const char* c = msg.c_str();
  bool success = np.produce(c, msg.size(), true);
  EXPECT_TRUE(success);
}
