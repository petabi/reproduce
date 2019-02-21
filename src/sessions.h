#ifndef SESSIONS_H
#define SESSIONS_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "forward_proto.h"

// Potential varying status for sampling...most for future use
enum class Sampling_status { no_sampling = -1, sample };

class Sessions {
public:
  void make_next_message(PackMsg& msg);
  void update_session(uint32_t src, uint32_t dst, uint8_t proto, uint16_t sport,
                      uint16_t dport, const char* data, size_t len);

  static constexpr size_t min_sample_size = 128;
  static constexpr size_t max_sample_size = 2048;
  static constexpr size_t max_message_size = 131072; // up to 128 KB messages

private:
  size_t message_data = 0;
  size_t message_id = 0;
  std::unordered_map<uint64_t,
                     std::pair<Sampling_status, std::vector<unsigned char>>>
      session_map;
};

inline uint64_t hash_key(uint32_t src, uint32_t dst, uint8_t proto,
                         uint16_t sport, uint16_t dport)
{
  return ((static_cast<uint64_t>(src) + static_cast<uint64_t>(dst)) << 31) +
         (static_cast<uint64_t>(proto) << 17) +
         (static_cast<uint64_t>(sport) + static_cast<uint64_t>(dport));
}
#endif

// vim: et:ts=2:sw=2
