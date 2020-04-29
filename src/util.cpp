#include <cstdarg>
#include <cstring>
#include <sys/stat.h>

#include "util.h"

using namespace std;

auto Util::ltrim(std::string& str) -> std::string&
{
  str.erase(0, str.find_first_not_of("\t\n\v\f\r "));
  return str;
}

auto Util::rtrim(std::string& str) -> std::string&
{
  str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1);
  return str;
}

auto Util::trim(std::string& str) -> std::string& { return ltrim(rtrim(str)); }

// vim: et:ts=2:sw=2
