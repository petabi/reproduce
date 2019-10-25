#include <arpa/inet.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "event.h"
#include "forward_proto.h"

PackMsg::PackMsg() : fm(forward_mode_new()), buffer(serialization_buffer_new())
{
}

PackMsg::~PackMsg()
{
  serialization_buffer_free(buffer);
  forward_mode_free(fm);
};

std::pair<const char*, size_t> PackMsg::pack()
{
  forward_mode_serialize(fm, buffer);
  return std::make_pair(
      reinterpret_cast<const char*>(serialization_buffer_data(buffer)),
      serialization_buffer_len(buffer));
}

void PackMsg::tag(const std::string& str)
{
  if (!forward_mode_set_tag(fm, str.data())) {
    throw "invalid tag";
  }
}

void PackMsg::entry(const uint64_t& id, const std::string& str,
                    const std::vector<unsigned char>& vec)
{
  if (!forward_mode_append_raw(fm, id, str.data(), vec.data(), vec.size())) {
    throw "invalid raw entry";
  }
}

void PackMsg::entry(const uint64_t& id, const std::string& str,
                    const std::vector<unsigned char>& vec, uint32_t src,
                    uint32_t dst, uint16_t sport, uint16_t dport, uint8_t proto)
{
  if (!forward_mode_append_packet(fm, id, str.data(), vec.data(), vec.size(),
                                  src, dst, sport, dport, proto)) {
    throw "invalid entry";
  }
}

void PackMsg::option(const std::string& option, const std::string& value)
{
  if (!forward_mode_add_option(fm, option.data(), value.data())) {
    throw "invalid option";
  }
}

void PackMsg::clear() { forward_mode_clear(fm); }
size_t PackMsg::get_bytes() const { return forward_mode_serialized_len(fm); }
size_t PackMsg::get_entries() const { return forward_mode_size(fm); };
