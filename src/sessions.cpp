#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "forward_proto.h"
#include "sessions.h"

void Sessions::make_next_message(PackMsg& msg)
{
  for (auto s : session_map) {
    if (s.second.second.size() >= min_sample_size) {
      msg.entry(message_id++, "message", s.second.second);
      message_data -= s.second.second.size();
      s.second.first = Sampling_status::no_sampling;
      s.second.second.clear();
    }
  }
}

void Sessions::update_session(uint32_t src, uint32_t dst, uint8_t proto,
                              uint16_t sport, uint16_t dport, const char* data,
                              size_t len)
{
  uint64_t hash = hash_key(src, dst, proto, sport, dport);
  auto it = session_map.find(hash);
  if (it != session_map.end()) {
    if (it->second.first < Sampling_status::sample ||
        it->second.second.size() >= max_sample_size) {
      return;
    }
    size_t read_len = std::min(len, max_sample_size - it->second.second.size());
    it->second.second.insert(it->second.second.end(), data, data + read_len);
    message_data += read_len;
  } else {
    size_t read_len = std::min(len, max_sample_size);
    std::vector<unsigned char> temp(data, data + read_len);
    session_map[hash] = std::make_pair(Sampling_status::sample, temp);
    if (read_len < max_sample_size) {
      session_map[hash].second.reserve(max_sample_size);
    }
    message_data += read_len;
  }
}
