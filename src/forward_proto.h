#ifndef FORWARD_PROTO_H
#define FORWARD_PROTO_H

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "event.h"
#include "producer.h"

static constexpr char src_key[] = "src";
static constexpr char dst_key[] = "dst";
static constexpr char sport_key[] = "sport";
static constexpr char dport_key[] = "dport";
static constexpr char proto_key[] = "proto";
static constexpr char session_msg_fmt[] =
    R"([,{"":"","":"","":"","":"","":"","":""}],)";
static constexpr size_t session_extra_bytes =
    (sizeof(src_key) - 1) + (sizeof(dst_key) - 1) + (sizeof(dport_key) - 1) +
    (sizeof(sport_key) - 1) + (sizeof(proto_key) - 1) +
    (sizeof(session_msg_fmt) - 1) + (2 * sizeof(uint32_t)) +
    (2 * sizeof(uint16_t)) + sizeof(uint8_t);

class PackMsg {
public:
  PackMsg();
  PackMsg(const PackMsg&) = delete;
  PackMsg& operator=(const PackMsg&) = delete;
  PackMsg(PackMsg&&) = delete;
  PackMsg& operator=(const PackMsg&&) = delete;
  ~PackMsg();

  std::pair<const char*, size_t> pack();
  void tag(const std::string& str);
  void entry(const uint64_t& id, const std::string& str,
             const std::vector<unsigned char>& vec);
  void entry(const uint64_t& id, const std::string& str,
             const std::vector<unsigned char>& vec, uint32_t src, uint32_t dst,
             uint16_t sport, uint16_t dport, uint8_t proto);
  void option(const std::string& option, const std::string& value);
  void clear();
  size_t get_bytes() const;
  size_t get_entries() const;
  size_t get_max_bytes() const { return max_bytes; }
  void set_max_bytes(size_t mb) { max_bytes = mb; }

private:
  size_t bytes = 0;
  size_t max_bytes = default_produce_max_bytes;
  ForwardMode* fm;
  SerializationBuffer* buffer;
};

#endif
