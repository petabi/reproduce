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

#endif
