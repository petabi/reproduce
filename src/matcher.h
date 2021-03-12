#ifndef MATCHER_H
#define MATCHER_H

#include <string>

struct InnerMatcher;

extern "C" {

void matcher_free(InnerMatcher* ptr);
auto matcher_match(InnerMatcher* ptr, const char* data, size_t len) -> size_t;
auto matcher_new(const char* filename) -> InnerMatcher*;

} // extern "C"

class Matcher {
public:
  Matcher() = delete;
  Matcher(const std::string& filename);
  Matcher(const Matcher&) = delete;
  auto operator=(const Matcher&) -> Matcher& = delete;
  Matcher(Matcher&&) = delete;
  auto operator=(const Matcher &&) -> Matcher& = delete;
  ~Matcher();
  auto match(const char* content, size_t content_length) -> bool;

private:
  InnerMatcher* inner{nullptr};
};

#endif
