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
  Matcher& operator=(const Matcher&) = delete;
  Matcher(Matcher&&) = delete;
  Matcher& operator=(const Matcher&&) = delete;
  ~Matcher();
  bool match(const char* content, size_t content_length);
  bool reload(const std::string& filename);

private:
  hs_database_t* hs_db{nullptr};
  hs_scratch_t* scratch{nullptr};
  Mode mode;

  static int on_match(unsigned int id, unsigned long long from,
                      unsigned long long to, unsigned int flags, void* ctx);
  enum class MatchResult { TRUE, FALSE, ERROR };
  MatchResult scan_block(const char* content, size_t content_length,
                         const match_event_handler& onEvent);
  void databases_from_file(const std::string& filename);
  void parse_file(const std::string& filename,
                  std::vector<std::string>& patterns,
                  std::vector<unsigned int>& flags,
                  std::vector<unsigned int>& ids);
  unsigned int parse_flags(const std::string& flagsStr);
  hs_database_t* build_database(const std::vector<const char*>& expressions,
                                const std::vector<unsigned int>& flags,
                                const std::vector<unsigned>& ids,
                                unsigned int mode);
};

#endif
