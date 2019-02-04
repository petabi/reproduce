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
  Config conf;
  conf.queue_size = 1;
  conf.queue_period = 1;
  conf.input = "Log";
  conf.output = "mytestfile.log";
  conf.kafka_broker = "[localhost:9001]";
  conf.kafka_topic = "test_topic";
  conf.input_type = InputType::Log;
  conf.output_type = OutputType::File;
  auto shared_conf = std::make_shared<Config>(conf);
  KafkaProducer kp(shared_conf);
  size_t mb = kp.get_max_bytes();
  EXPECT_GT(mb, 0);
  // Need to test produce but avoid eternal wait.
  // Also, test causes an abort as is.
}

TEST(test_producer, test_producer_file)
{
  Config conf;
  conf.queue_size = 1;
  conf.queue_period = 1;
  conf.input = "Log";
  conf.output = "mytestfile.log";
  conf.input_type = InputType::Log;
  conf.output_type = OutputType::File;
  auto shared_conf = std::make_shared<Config>(conf);
  FileProducer fp(shared_conf);
  size_t mb = fp.get_max_bytes();
  EXPECT_GT(mb, 0);
  std::string mymessage = "Here is a message to produce!";
  bool success = fp.produce(mymessage, true);
  EXPECT_TRUE(success);
  std::ifstream myfile(conf.output);
  std::vector<char> mybuffer;
  // getline appends a \0 at the end.
  mybuffer.resize(mymessage.size() + 1, 0);
  myfile.getline(mybuffer.data(),
                 static_cast<std::streamsize>(mybuffer.size()));
  EXPECT_STREQ(mymessage.c_str(), mybuffer.data());
  std::remove(mymessage.c_str());
}

TEST(test_producer, test_producer_null)
{
  Config conf;
  auto shared_conf = std::make_shared<Config>(conf);
  NullProducer np(shared_conf);
  size_t mb = np.get_max_bytes();
  EXPECT_GT(mb, 0);
  bool success = np.produce("A message to lose!");
  EXPECT_TRUE(success);
}
