#include <cstdarg>
#include <cstring>
#include <sys/stat.h>

#include "util.h"

using namespace std;

bool Util::debug = false;

void Util::mprint(const char* message, const size_t count) noexcept
{
  if (!debug) {
    return;
  }

  cout << "[" << count << "] " << message << "\n";
}

void Util::set_debug(const bool& _debug) { debug = _debug; }

std::string Util::del_space(std::string& str)
{
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] == ' ') {
      str.erase(i, 1);
      i--;
    }
  }

  return str;
}

// vim: et:ts=2:sw=2
