#include <string>

#include <gtest/gtest.h>

#include "entropy_calculator.h"

TEST(test_entropy, test_entropy_basic)
{
  Entropy_calculator ec;
  std::string test_string = "abcd";
  EXPECT_DOUBLE_EQ(2.0, ec.calculate_entropy(reinterpret_cast<unsigned char*>(
                                                 test_string.data()),
                                             test_string.size()));
  EXPECT_DOUBLE_EQ(
      ec.max_entropy_for_size(test_string.size()),
      ec.calculate_entropy(reinterpret_cast<unsigned char*>(test_string.data()),
                           test_string.size()));
  test_string = "aaaa";
  EXPECT_DOUBLE_EQ(0.0, ec.calculate_entropy(reinterpret_cast<unsigned char*>(
                                                 test_string.data()),
                                             test_string.size()));
  test_string = "aabb";
  EXPECT_DOUBLE_EQ(1.0, ec.calculate_entropy(reinterpret_cast<unsigned char*>(
                                                 test_string.data()),
                                             test_string.size()));
  test_string = "aaaaaaabbb";
  EXPECT_NEAR(
      0.88,
      ec.calculate_entropy(reinterpret_cast<unsigned char*>(test_string.data()),
                           test_string.size()),
      0.01);

  test_string = "aaaaabbbcc";
  EXPECT_NEAR(
      1.4,
      ec.calculate_entropy(reinterpret_cast<unsigned char*>(test_string.data()),
                           test_string.size()),
      0.09);
  test_string = "aabccdeefgghiijk";
  double e = ec.calculate_entropy(
      reinterpret_cast<unsigned char*>(test_string.data()), test_string.size());
  double ratio = e / ec.max_entropy_for_size(test_string.size());
  EXPECT_GT(ratio, 0.75);
  test_string = "aaaaabbbbcccddef";
  e = ec.calculate_entropy(reinterpret_cast<unsigned char*>(test_string.data()),
                           test_string.size());
  ratio = e / ec.max_entropy_for_size(test_string.size());
  EXPECT_LT(ratio, 0.75);
}
