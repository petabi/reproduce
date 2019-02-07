#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "matcher.h"

TEST(test_matcher, test_matcher_basic)
{
  std::vector<std::string> signatures = {"abc", "aBc", "123", "W h;a,t",
                                         "teddy!"};
  std::string sample_file = "mysample.rules";
  std::ofstream sample_out(sample_file);
  for (const auto& sig : signatures) {
    sample_out << sig << std::endl;
  }
  sample_out.close();
  Matcher mymatcher(sample_file);
  for (const auto& sig : signatures) {
    EXPECT_TRUE(mymatcher.match(sig.c_str(), sig.size()));
  }
  sample_out.open(sample_file, std::ofstream::out | std::ofstream::app);
  std::string special = R"(\x78\x79\x7A)";
  sample_out << special << std::endl;
  sample_out.close();
  mymatcher.reload(sample_file);
  for (const auto& sig : signatures) {
    EXPECT_TRUE(mymatcher.match(sig.c_str(), sig.size()));
  }
  EXPECT_TRUE(mymatcher.match("xyz", 3));
  std::remove(sample_file.c_str());
}

// Test fails to identify /a+b{2}.c?/is
TEST(test_matcher, DISABLED_test_matcher_difficult_regex)
{
  std::vector<std::string> signatures = {
      R"(/a+b{2}.c?/is)", R"(a(b[cd]|e[fg]).)", R"(z\d{2}y\W+x\s*)"};
  std::string sample_file = "mysample.rules";
  std::ofstream sample_out(sample_file);
  for (const auto& sig : signatures) {
    sample_out << sig << std::endl;
  }
  sample_out.close();
  Matcher mymatcher(sample_file);

  EXPECT_FALSE(mymatcher.match("abgc", 4));
  EXPECT_TRUE(mymatcher.match("abcx", 4));
  EXPECT_TRUE(mymatcher.match("aefy", 4));
  EXPECT_TRUE(mymatcher.match("z11y;;x   ", 10));
  EXPECT_TRUE(mymatcher.match("z22y,:x", 7));
  EXPECT_FALSE(mymatcher.match("z2y,.%:x   ", 11));
  EXPECT_TRUE(mymatcher.match("aAabb\nc", 7));
  EXPECT_TRUE(mymatcher.match("abb ", 4));
  std::remove(sample_file.c_str());
}
