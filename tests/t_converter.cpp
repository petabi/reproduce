#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "converter.h"
#include "forward_proto.h"
#include "matcher.h"

TEST(test_converter, test_packet_converter)
{
  std::vector<unsigned char> mypkt1 = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x39, 0x00, 0x00, 0x00, 0x39, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00, 0x00,
      0x2B, 0x00, 0x01, 0x00, 0x00, 0x40, 0x06, 0x7C, 0xCA, 0x7F, 0x00,
      0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x00, 0x14, 0x00, 0x50, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00,
      0xCD, 0x16, 0x00, 0x00, 0x61, 0x62, 0x63};
  std::vector<unsigned char> mypkt2 = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x39, 0x00, 0x00, 0x00, 0x39, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00, 0x00,
      0x2B, 0x00, 0x01, 0x00, 0x00, 0x40, 0x06, 0x7C, 0xCA, 0x7F, 0x00,
      0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x00, 0x14, 0x00, 0x50, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00,
      0xCD, 0x16, 0x00, 0x00, 0x31, 0x32, 0x33};
  std::vector<std::string> signatures = {"abc", "xyz"};
  std::string sample_file = "mysample.rules";
  std::ofstream sample_out(sample_file);
  for (const auto& sig : signatures) {
    sample_out << sig << std::endl;
  }
  sample_out.close();
  PacketConverter pktcon(1);
  pktcon.set_matcher(sample_file, Mode::BLOCK);
  Matcher* mymatcher = pktcon.get_matcher();
  ASSERT_FALSE(mymatcher == nullptr);
  std::remove(sample_file.c_str());
  PackMsg pmsg;
  Conv::Status mystatus = pktcon.convert(reinterpret_cast<char*>(mypkt1.data()),
                                         mypkt1.size(), pmsg);
  EXPECT_EQ(mystatus, Conv::Status::Pass);
  mystatus = pktcon.convert(reinterpret_cast<char*>(mypkt2.data()),
                            mypkt2.size(), pmsg);
  EXPECT_EQ(mystatus, Conv::Status::Success);
  EXPECT_EQ(pmsg.get_entries(), 1);
}

TEST(test_converter, test_log_converter)
{
  std::string msg1 = "here is my message abc";
  std::string msg2 = "123 message 2 should not match!";
  std::vector<std::string> signatures = {"abc", "xyz"};
  std::string sample_file = "mysample.rules";
  std::ofstream sample_out(sample_file);
  for (const auto& sig : signatures) {
    sample_out << sig << std::endl;
  }
  sample_out.close();
  LogConverter logcon;
  logcon.set_matcher(sample_file, Mode::BLOCK);
  Matcher* mymatcher = logcon.get_matcher();
  ASSERT_FALSE(mymatcher == nullptr);
  std::remove(sample_file.c_str());
  PackMsg pmsg;
  Conv::Status mystatus =
      logcon.convert(reinterpret_cast<char*>(msg1.data()), msg1.size(), pmsg);
  EXPECT_EQ(mystatus, Conv::Status::Pass);
  mystatus =
      logcon.convert(reinterpret_cast<char*>(msg2.data()), msg2.size(), pmsg);
  EXPECT_EQ(mystatus, Conv::Status::Success);
  EXPECT_EQ(pmsg.get_entries(), 1);
}

TEST(test_converter, test_null_converter)
{
  std::string msg = "No message shall match!";
  NullConverter ncon;
  PackMsg pmsg;
  Conv::Status mystatus =
      ncon.convert(reinterpret_cast<char*>(msg.data()), msg.size(), pmsg);
  EXPECT_EQ(mystatus, Conv::Status::Success);
  EXPECT_EQ(pmsg.get_entries(), 0); // Is zero correct here?
}
