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
  uint32_t src = 1;
  uint32_t dst = 2;
  uint8_t proto = 3;
  uint16_t sport = 4;
  uint16_t dport = 5;
  std::string mymessage = "my message number: ";
  for (int i = 1; i < 10; ++i) {
    std::string temp = mymessage + std::to_string(i) + "!";
    mysession.update_session(src, dst, proto, sport, dport, temp.data(),
                             temp.size());
  }
  PackMsg pm;
  mysession.make_next_message(pm);
  std::stringstream mystream;
  pm.pack(mystream);
  EXPECT_EQ(1, pm.get_entries());
  EXPECT_EQ(231, pm.get_bytes());
  auto mypmsgstring = pm.get_string(mystream);
  for (int i = 1; i < 10; ++i) {
    std::string temp = mymessage + std::to_string(i);
    EXPECT_TRUE(mypmsgstring.find(temp));
  }
}
