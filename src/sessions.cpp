#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "entropy_calculator.h"
#include "forward_proto.h"
#include "sessions.h"

using namespace std;

size_t Sessions::make_next_message(PackMsg& msg, uint64_t event_id)
{
  std::vector<uint64_t> removal;
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
      if ((msg.get_bytes() + s.second.data.size() + session_extra_bytes +
           message_n_label_bytes) > msg.get_max_bytes()) {
        continue;
      }
      msg.entry(event_id, message_label, s.second.data, s.second.src,
                s.second.dst, s.second.sport, s.second.dport, s.second.proto);
      s.second.bytes_sampled += s.second.data.size();
      s.second.age = 0;
      if (s.second.bytes_sampled >= max_sample_size) {
        s.second.status = Sampling_status::no_sampling;
      }
      message_data -= s.second.data.size();
      s.second.data.clear();
      if (msg.get_bytes() >= msg.get_max_bytes()) {
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
  return 0;
}

bool Sessions::update_session(uint32_t src, uint32_t dst, uint8_t proto,
                              uint16_t sport, uint16_t dport, const char* data,
                              size_t len)
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
                                sport, dport, proto, temp);
    if (read_len < max_sample_size) {
      session_map[hash].data.reserve(max_sample_size);
    }
    newsession = true;
  }
  message_data += read_len;
  return newsession;
}
