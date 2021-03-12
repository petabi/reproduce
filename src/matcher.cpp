#include "matcher.h"
#include "util.h"

Matcher::Matcher(const std::string& filename)
{
  inner = matcher_new(filename.c_str());
}

Matcher::~Matcher() { matcher_free(inner); }

auto Matcher::match(const char* content, size_t content_length) -> bool
{
  if (content_length == 0) {
    return false;
  }
  size_t ret = matcher_match(inner, content, content_length);
  return ret > 0;
}
