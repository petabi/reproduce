#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <entropy_calculator.h>

double Entropy_calculator::calculate_entropy(const unsigned char* data,
                                             size_t num_bytes)
{
  for (size_t i = 0; i < num_bytes; ++i) {
    frequency_map[static_cast<size_t>(data[i])]++;
  }
  double entropy = 0.0;
  double p = 0.0;
  for (auto& f : frequency_map) {
    if (f != 0) {
      p = static_cast<double>(f) / static_cast<double>(num_bytes);
      entropy += p * std::log2(p);
      f = 0;
    }
  }
  return -entropy;
}

double Entropy_calculator::max_entropy_for_size(size_t num_bytes) const
{
  return std::log2(num_bytes);
}
