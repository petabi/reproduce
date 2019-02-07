#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "forward_proto.h"

TEST(test_forward_proto, test_pack_msg)
{
  PackMsg pmsg;
  std::stringstream mystream;
  std::string msg1 = "my very first message";
  std::string msg2 = "my second very message";
  std::vector<unsigned char> msg1_vec(msg1.begin(), msg1.end());
  std::vector<unsigned char> msg2_vec(msg2.begin(), msg2.end());
  pmsg.tag("Test");
  pmsg.option("option", "optional");
  pmsg.entry(1, "message", msg1_vec);
  pmsg.entry(2, "message", msg2_vec);
  pmsg.pack(mystream);
  EXPECT_GT(pmsg.get_bytes(), (msg1_vec.size() + msg2_vec.size()));
  EXPECT_EQ(pmsg.get_entries(), 2);
  std::string mymsgasstring =
      R"(["Test",[[1,{"message":"my very first message"}],)"
      R"([2,{"message":"my second very message"}]],)"
      R"({"option":"optional"}])";
  std::string unpacked_string = pmsg.get_string(mystream);
  pmsg.clear();
  EXPECT_GT(pmsg.get_bytes(), 0);
  EXPECT_EQ(pmsg.get_entries(), 0);
}
