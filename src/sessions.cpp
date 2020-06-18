#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "entropy_calculator.h"
#include "event.h"
#include "sessions.h"
#include "util.h"

using namespace std;

auto Sessions::make_next_message(ForwardMode* msg, uint64_t event_id,
                                 size_t max_bytes) -> size_t
{
  std::vector<uint64_t> removal;
  uint32_t seq_no = (event_id & 0x00000000FFFFFF00) >> 8;
  for (auto& s : session_map) {
    if (s.second.status != Sampling_status::sample ||
        s.second.data.size() < min_sample_size) {
      if (s.second.age >= max_age) {
        removal.push_back(s.first);
      }
      ++s.second.age;
      continue;
    }
    if ((e_calc.calculate_entropy(
             reinterpret_cast<const unsigned char*>(s.second.data.data()),
             s.second.data.size()) /
         e_calc.max_entropy_for_size(s.second.data.size())) < entropy_ratio) {
      if ((forward_mode_serialized_len(msg) + s.second.data.size() +
           SESSION_EXTRA_BYTES + message_n_label_bytes) > max_bytes) {
        continue;
      }

      forward_mode_append_packet(msg, s.second.session_event_id, message_label,
                                 s.second.data.data(), s.second.data.size(),
                                 s.second.src, s.second.dst, s.second.sport,
                                 s.second.dport, s.second.proto);
      s.second.bytes_sampled += s.second.data.size();
      s.second.age = 0;
      seq_no++;
      if (s.second.bytes_sampled >= max_sample_size) {
        s.second.status = Sampling_status::no_sampling;
      }
      message_data -= s.second.data.size();
      s.second.data.clear();
      if (forward_mode_serialized_len(msg) >= max_bytes) {
        break;
      }
    } else {
      s.second.status = Sampling_status::no_sampling;
      s.second.bytes_sampled = 0;
      message_data -= s.second.data.size();
      s.second.data.clear();
    }
  }
  if (!removal.empty()) {
    for (const auto& r : removal) {
      message_data -= session_map[r].data.size();
      session_map.erase(r);
    }
  }
  return ((event_id & 0xFFFFFFFF000000FF) | ((seq_no & 0x00FFFFFF) << 8));
}

auto Sessions::update_session(uint32_t src, uint32_t dst, uint8_t proto,
                              uint16_t sport, uint16_t dport, const char* data,
                              size_t len, uint64_t event_id) -> bool
{
  bool newsession = false;
  uint64_t hash = hash_key(src, dst, proto, sport, dport);
  auto it = session_map.find(hash);
  size_t read_len = std::min(len, max_sample_size);
  if (it != session_map.end()) {
    if (it->second.status != Sampling_status::sample ||
        (it->second.bytes_sampled + it->second.data.size()) >=
            max_sample_size) {
      return newsession;
    }
    read_len = std::min(read_len, max_sample_size - (it->second.bytes_sampled +
                                                     it->second.data.size()));
    it->second.data.insert(it->second.data.end(), data, data + read_len);
  } else {
    std::vector<unsigned char> temp(data, data + read_len);
    session_map[hash] = Session(read_len, Sampling_status::sample, src, dst,
                                sport, dport, proto, event_id, temp);
    if (read_len < max_sample_size) {
      session_map[hash].data.reserve(max_sample_size);
    }
    newsession = true;
  }
  message_data += read_len;
  return newsession;
}
