#ifndef SESSIONS_H
#define SESSIONS_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "entropy_calculator.h"

struct ForwardMode;

static constexpr char src_key[] = "src";
static constexpr char dst_key[] = "dst";
static constexpr char sport_key[] = "sport";
static constexpr char dport_key[] = "dport";
static constexpr char proto_key[] = "proto";
static constexpr size_t session_extra_bytes =
    (sizeof(src_key)) + (sizeof(dst_key)) + (sizeof(dport_key)) +
    (sizeof(sport_key)) + (sizeof(proto_key)) + 2 * 5 + (2 * sizeof(uint32_t)) +
    (2 * sizeof(uint16_t)) + sizeof(uint8_t);

// Potential varying status for sampling...most for future use
enum class Sampling_status { no_sampling = -1, sample };
constexpr auto message_label = "message";

// size of message_label includes null terminator.
constexpr size_t message_n_label_bytes =
    (sizeof(message_label) - 1) + sizeof(size_t);

struct Session {
  Session() = default;
  Session(size_t b, Sampling_status stat, uint32_t s, uint32_t d, uint16_t sp,
          uint16_t dp, uint8_t p, uint64_t eid, std::vector<unsigned char> dat)
      : bytes_sampled(b), status(stat), src(s), dst(d), sport(sp), dport(dp),
        proto(p), session_event_id(eid), data(std::move(dat))
  {
  }
  Session(const Session& other) = default;

  size_t age = 0;
  size_t bytes_sampled;
  Sampling_status status;
  uint32_t src;
  uint32_t dst;
  uint16_t sport;
  uint16_t dport;
  uint8_t proto;
  uint64_t session_event_id;
  std::vector<unsigned char> data;
};

class Sessions {
public:
  bool empty() const { return session_map.empty(); }
  size_t get_number_bytes_in_sessions() const { return message_data; }
  size_t make_next_message(ForwardMode* msg, uint64_t event_id,
                           size_t max_bytes);
  bool update_session(uint32_t src, uint32_t dst, uint8_t proto, uint16_t sport,
                      uint16_t dport, const char* data, size_t len,
                      uint64_t event_id);
  void set_allowed_entropy_ratio(float e) { entropy_ratio = e; }
  size_t size() const { return session_map.size(); }

  static constexpr size_t max_age = 128;
  static constexpr size_t max_sample_size = 2048;
  static constexpr size_t min_sample_size = 128;

private:
  Entropy_calculator e_calc;
  size_t message_data = 0;
  float entropy_ratio =
      0.9; // <= amount entropy vs max entropy allowed before drop
  std::unordered_map<uint64_t, Session> session_map;
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
