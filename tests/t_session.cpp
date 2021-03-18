#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "forward_proto.h"
#include "sessions.h"

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
