#ifndef MATCHER_H
#define MATCHER_H
#include <fstream>
#include <hs/hs.h>
#include <iostream>
#include <string>
#include <vector>

enum class Mode { BLOCK, STREAM };

class Matcher {
public:
  Matcher() = delete;
  Matcher(const std::string& filename, const Mode& mode = Mode::BLOCK);
  Matcher(const Matcher&) = delete;
  auto operator=(const Matcher&) -> Matcher& = delete;
  Matcher(Matcher&&) = delete;
  auto operator=(const Matcher &&) -> Matcher& = delete;
  ~Matcher();
  auto match(const char* content, size_t content_length) -> bool;
  auto reload(const std::string& filename) -> bool;

private:
  hs_database_t* hs_db{nullptr};
  hs_scratch_t* scratch{nullptr};
  Mode mode;

  static auto on_match(unsigned int id, unsigned long long from,
                       unsigned long long to, unsigned int flags, void* ctx)
      -> int;
  enum class MatchResult { TRUE, FALSE, ERROR };
  auto scan_block(const char* content, size_t content_length,
                  const match_event_handler& onEvent) -> MatchResult;
  void databases_from_file(const std::string& filename);
  void parse_file(const std::string& filename,
                  std::vector<std::string>& patterns,
                  std::vector<unsigned int>& flags,
                  std::vector<unsigned int>& ids);
  auto parse_flags(const std::string& flagsStr) -> unsigned int;
  auto build_database(const std::vector<const char*>& expressions,
                      const std::vector<unsigned int>& flags,
                      const std::vector<unsigned>& ids, unsigned int mode)
      -> hs_database_t*;
};

#endif
