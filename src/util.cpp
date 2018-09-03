#include <cstdarg>
#include <cstring>
#include <sys/stat.h>

#include "util.h"

using namespace std;

bool Util::debug = false;

void Util::mprint(const char* message) noexcept
{
  if (!debug) {
    return;
  }

  cout << message << "\n";
}

void Util::set_debug(const bool& _debug) { debug = _debug; }

// vim: et:ts=2:sw=2
