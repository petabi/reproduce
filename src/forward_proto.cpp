#include <arpa/inet.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "forward_proto.h"

static constexpr char fix_mark[] = R"(["",[],{"":""}])";

PackMsg::PackMsg() : bytes{sizeof(fix_mark)} {}

PackMsg::~PackMsg() = default;

void PackMsg::pack(std::stringstream& ss) { msgpack::pack(ss, fm); }

void PackMsg::tag(const std::string& str)
{
  fm.tag = str;
  bytes += str.length();
}

void PackMsg::entry(const uint64_t& id, const std::string& str,
                    const std::vector<unsigned char>& vec)
{
  std::map<std::string, std::vector<unsigned char>> msg;
  msg.insert(std::make_pair(str, vec));
  fm.entries.emplace_back(id, msg);
  bytes += (sizeof(id) + str.length() + vec.size() +
            std::string(R"([,{"":""}],)").length());
}

void PackMsg::entry(const uint64_t& id, const std::string& str,
                    const std::vector<unsigned char>& vec, uint32_t src,
                    uint32_t dst, uint16_t sport, uint16_t dport, uint8_t proto)
{
  std::map<std::string, std::vector<unsigned char>> msg;
  msg.insert(std::make_pair(str, vec));
  src = htonl(src);
  msg.insert(std::make_pair(
      src_key, std::vector<unsigned char>(
                   reinterpret_cast<unsigned char*>(&src),
                   reinterpret_cast<unsigned char*>(&src) + sizeof(src))));
  dst = htonl(dst);
  msg.insert(std::make_pair(
      dst_key, std::vector<unsigned char>(
                   reinterpret_cast<unsigned char*>(&dst),
                   reinterpret_cast<unsigned char*>(&dst) + sizeof(dst))));
  sport = htons(sport);
  msg.insert(std::make_pair(
      sport_key,
      std::vector<unsigned char>(reinterpret_cast<unsigned char*>(&sport),
                                 reinterpret_cast<unsigned char*>(&sport) +
                                     sizeof(sport))));
  dport = htons(dport);
  msg.insert(std::make_pair(
      dport_key,
      std::vector<unsigned char>(reinterpret_cast<unsigned char*>(&dport),
                                 reinterpret_cast<unsigned char*>(&dport) +
                                     sizeof(dport))));
  msg.insert(std::make_pair(
      proto_key,
      std::vector<unsigned char>(reinterpret_cast<unsigned char*>(&proto),
                                 reinterpret_cast<unsigned char*>(&proto) +
                                     sizeof(proto))));
  fm.entries.emplace_back(id, msg);
  bytes += (sizeof(id) + str.length() + vec.size() + session_extra_bytes);
}

void PackMsg::option(const std::string& option, const std::string& value)
{
  fm.option[option] = value;
  bytes += (option.length() + value.length());
}

void PackMsg::clear()
{
  fm.tag = "";
  fm.option.clear();
  fm.entries.clear();
  bytes = sizeof(fix_mark);
}

size_t PackMsg::get_bytes() const { return bytes; }
size_t PackMsg::get_entries() const { return fm.entries.size(); };

std::string PackMsg::get_string(const std::stringstream& ss) const
{
  msgpack::object_handle oh = msgpack::unpack(ss.str().data(), ss.str().size());
  msgpack::object obj = oh.get();

  std::stringstream buffer;
  buffer << obj;
  return buffer.str();
}
