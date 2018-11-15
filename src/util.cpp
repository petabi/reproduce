#include <cstdarg>
#include <cstring>
#include <sys/stat.h>

#include "util.h"

using namespace std;

bool Util::debug = false;

void Util::set_debug(const bool& _debug) { debug = _debug; }

std::string& Util::ltrim(std::string& str)
{
  str.erase(0, str.find_first_not_of("\t\n\v\f\r "));
  return str;
}

std::string& Util::rtrim(std::string& str)
{
  str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1);
  return str;
}

std::string& Util::trim(std::string& str) { return ltrim(rtrim(str)); }

// vim: et:ts=2:sw=2
