#ifndef FORWARD_PROTO_H
#define FORWARD_PROTO_H

#include <iostream>
#include <list>
#include <map>
#include <msgpack.hpp>
#include <string>
#include <tuple>
#include <vector>

class ForwardMsg {
public:
  std::string tag;
  // clang-format off
  std::vector<std::tuple<size_t, std::map<std::string, std::vector<unsigned char> > > > entries;
  // clang-format on
  std::map<std::string, std::string> option;
  MSGPACK_DEFINE(tag, entries, option);
};

class PackMsg {
public:
  PackMsg();
  PackMsg(const PackMsg&) = delete;
  PackMsg& operator=(const PackMsg&) = delete;
  PackMsg(PackMsg&&) = delete;
  PackMsg& operator=(const PackMsg&&) = delete;
  ~PackMsg();

  void pack(std::stringstream& ss);
  std::string unpack(const std::stringstream& ss);
  void tag(const std::string& str);
  void entry(const size_t& id, const std::string& str,
             const std::vector<unsigned char>& vec);
  void option(const std::string& option, const std::string& value);
  void clear();
  size_t get_bytes() const;
  size_t get_entries() const;
  std::string get_string(const std::stringstream& ss) const;

private:
  size_t bytes = 0;
  ForwardMsg fm;
};

#endif
