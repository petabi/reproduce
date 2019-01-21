#include "forward_proto.h"

static constexpr char fix_mark[] = R"(["",[],{"":""}])";

PackMsg::PackMsg() { bytes = sizeof(fix_mark); }

PackMsg::~PackMsg() = default;

void PackMsg::pack(std::stringstream& ss) { msgpack::pack(ss, fm); }

void PackMsg::tag(const std::string& str)
{
  fm.tag = str;
  bytes += str.length();
}

void PackMsg::entry(const size_t& id, const std::string& str,
                    const std::vector<unsigned char>& vec)
{
  std::map<std::string, std::vector<unsigned char>> msg;
  msg.insert(std::make_pair(str, vec));
  fm.entries.push_back(std::make_tuple(id, msg));
  bytes += (sizeof(id) + str.length() + vec.size() +
            std::string(R"([,{"":""}],)").length());
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
