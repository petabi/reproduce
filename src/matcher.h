#ifndef MATCHER_H
#define MATCHER_H

#include <string>

struct Matcher;

extern "C" {

void matcher_free(Matcher* ptr);
auto matcher_match(Matcher* ptr, const char* data, size_t len) -> size_t;
auto matcher_new(const char* filename) -> Matcher*;

} // extern "C"

#endif
