#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "forward_proto.h"
#include "sessions.h"

TEST(test_session, test_hash)
{
  uint32_t src = 1;
  uint32_t dst = 2;
  uint8_t proto = 3;
  uint16_t sport = 4;
  uint16_t dport = 5;
  uint64_t key = hash_key(src, dst, proto, sport, dport);
  EXPECT_EQ(key, 6442844169);

  src = 0xFFFFFFFF;
  dst = 0xFFFFFFFF;
  proto = 0xFF;
  sport = 0xFFFF;
  dport = 0xFFFF;
  key = hash_key(src, dst, proto, sport, dport);
  EXPECT_EQ(key, 0xFFFFFFFF01FFFFFE);

  src = 0;
  dst = 0;
  proto = 0;
  sport = 0;
  dport = 0;
  key = hash_key(src, dst, proto, sport, dport);
  EXPECT_EQ(key, 0);

  src = 0x01020304;
  dst = 0x05060708;
  proto = 0x11;
  sport = 0x8090;
  dport = 0x6070;
  uint64_t key1 = hash_key(src, dst, proto, sport, dport);
  uint64_t key2 = hash_key(dst, src, proto, dport, sport);
  EXPECT_EQ(key1, key2);
}

TEST(test_session, test_update_session)
{
  Sessions mysession;
  uint32_t src = 0x61626364;
  uint32_t dst = 0x31323334;
  uint8_t proto = 0x7A;
  uint16_t sport = 0x4142;
  uint16_t dport = 0x3839;
  uint64_t event_id = 10;
  std::string mymessage = "my message number: ";
  for (int i = 1; i < 10; ++i) {
    std::string temp = mymessage + std::to_string(i) + "!";
    mysession.update_session(src, dst, proto, sport, dport, temp.data(),
                             temp.size(), event_id);
  }
  PackMsg pm;
  size_t initial_size = pm.get_bytes();
  size_t message_id_before = 0;
  size_t message_id_after = mysession.make_next_message(pm, message_id_before);
  EXPECT_EQ(1, message_id_after - message_id_before);

  std::stringstream mystream;
  pm.pack(mystream);
  EXPECT_EQ(1, pm.get_entries());

  // 9 iterations of (mymessage + 1 char id + !)
  size_t expected_size = session_extra_bytes + ((mymessage.size() + 2) * 9) +
                         initial_size + message_n_label_bytes;
  EXPECT_EQ(expected_size, pm.get_bytes());
  auto mypmsgstring = pm.get_string(mystream);
  for (int i = 1; i < 10; ++i) {
    std::string temp = mymessage + std::to_string(i);
    EXPECT_TRUE(mypmsgstring.find(temp) != std::string::npos);
  }
  EXPECT_TRUE(mypmsgstring.find(R"("src":"abcd")") != std::string::npos);
  EXPECT_TRUE(mypmsgstring.find(R"("dst":"1234")") != std::string::npos);
  EXPECT_TRUE(mypmsgstring.find(R"("sport":"AB")") != std::string::npos);
  EXPECT_TRUE(mypmsgstring.find(R"("dport":"89")") != std::string::npos);
  EXPECT_TRUE(mypmsgstring.find(R"("proto":"z")") != std::string::npos);
}

TEST(test_session, test_delete_session)
{

  uint32_t src = 1;
  uint32_t dst = 2;
  uint8_t proto = 3;
  uint16_t sport = 4;
  uint16_t dport = 5;
  uint64_t event_id = 6;
  Sessions mysession;
  std::string mymessage = "my message number: ";
  mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                           mymessage.size(), event_id);
  PackMsg pm;

  // Test that counter will eventually delete the session.
  for (size_t i = 0; i < mysession.max_age; ++i) {
    EXPECT_EQ(mysession.get_number_bytes_in_sessions(), mymessage.size());
    EXPECT_EQ(mysession.make_next_message(pm, 0), 0);
    EXPECT_FALSE(mysession.empty());
  }
  EXPECT_EQ(mysession.make_next_message(pm, 0), 0);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 0);
  EXPECT_TRUE(mysession.empty());

  // Test new sample will restart counter to delete session.
  for (size_t i = 1;
       mysession.get_number_bytes_in_sessions() < mysession.min_sample_size;
       ++i) {
    mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                             mymessage.size(), event_id);
    EXPECT_FALSE(mysession.empty());
    EXPECT_EQ(mysession.get_number_bytes_in_sessions(), i * mymessage.size());
  }
  EXPECT_EQ(mysession.make_next_message(pm, 0), 1);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 0);
  for (size_t i = 1;
       mysession.get_number_bytes_in_sessions() < mysession.min_sample_size;
       ++i) {
    mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                             mymessage.size(), event_id);
    EXPECT_FALSE(mysession.empty());
  }
  EXPECT_EQ(mysession.make_next_message(pm, 1), 2);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 0);
  mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                           mymessage.size(), event_id);
  for (size_t i = 0; i < mysession.max_age; ++i) {
    EXPECT_EQ(mysession.get_number_bytes_in_sessions(), mymessage.size());
    EXPECT_EQ(mysession.make_next_message(pm, 2), 2);
    EXPECT_FALSE(mysession.empty());
  }
  EXPECT_EQ(mysession.make_next_message(pm, 2), 2);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 0);
  EXPECT_TRUE(mysession.empty());
}

TEST(test_session, test_size)
{
  Sessions mysession;
  uint32_t src = 0x61626364;
  uint32_t dst = 0x31323334;
  uint8_t proto = 0x7A;
  uint16_t sport = 0x4142;
  uint16_t dport = 0x3839;
  uint64_t event_id = 1;
  std::string mymessage;
  for (int i = 0; i < 128; ++i) {
    mymessage.append("h");
  }
  PackMsg pm;
  size_t initial_size = pm.get_bytes();
  mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                           mymessage.size(), event_id);
  mysession.make_next_message(pm, 0x3132333435363738);

  // size of src, dst, sport, dport, proto, src_key, dst_key, sport_key
  // dport_key, proto_key, and session_msg_fmt
  size_t expected_session_bytes_size =
      4 + 4 + 2 + 2 + 1 + 3 + 3 + 5 + 5 + 5 + 41;
  EXPECT_EQ(session_extra_bytes, expected_session_bytes_size);
  size_t expected_size = session_extra_bytes + initial_size + mymessage.size() +
                         message_n_label_bytes;
  EXPECT_EQ(expected_size, pm.get_bytes());
}

TEST(test_session, test_max_size)
{
  Sessions mysession;
  uint32_t src = 0x61626364;
  uint32_t dst = 0x31323334;
  uint8_t proto = 0x7A;
  uint16_t sport = 0x4142;
  uint16_t dport = 0x3839;
  std::string mymessage;
  for (int i = 0; i < 128; ++i) {
    mymessage.append("h");
  }
  PackMsg pm;
  pm.set_max_bytes(64);
  mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                           mymessage.size(), 1);
  mysession.make_next_message(pm, 0x3132333435363738);
  EXPECT_EQ(pm.get_entries(), 0);
  pm.set_max_bytes(256);
  mysession.make_next_message(pm, 0x3132333435363738);
  EXPECT_EQ(pm.get_entries(), 1);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 0);
  pm.clear();
  mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                           mymessage.size(), 1);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 128);
  mysession.update_session(src, dst, proto, sport, dport, mymessage.data(),
                           mymessage.size(), 1);
  EXPECT_EQ(mysession.get_number_bytes_in_sessions(), 256);
  mysession.make_next_message(pm, 0x3132333435363738);

  EXPECT_EQ(pm.get_entries(), 0);
  pm.set_max_bytes(384);
  mysession.make_next_message(pm, 0x3132333435363738);
  EXPECT_EQ(pm.get_entries(), 1);
}
