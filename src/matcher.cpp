#include "matcher.h"
#include "util.h"

Matcher::Matcher(const std::string& filename, const Mode& _mode) : mode(_mode)
{
  databases_from_file(filename.c_str(), &hs_db);
  if (hs_alloc_scratch(hs_db, &scratch) != HS_SUCCESS) {
    throw std::runtime_error("failed to allocate pattern scratch space");
  }
}

Matcher::~Matcher()
{
  hs_free_database(hs_db);
  hs_free_scratch(scratch);
}

bool Matcher::match(const char* content, size_t content_length)
{
  if (content_length == 0) {
    return false;
  }
  match_event_handler onEvent = &Matcher::on_match;
  if (mode == Mode::BLOCK) {
    MatchResult ret = scan_block(content, content_length, onEvent);
    if (ret == MatchResult::TRUE) {
      return true;
    } else if (ret == MatchResult::FALSE) {
      return false;
    } else {
      // hyperscan matching error
      return false;
    }
  }
  return false;
// TODO: Implementation to scan stream
#if 0
    else {
      scanStream(content, content_length);
    }
#endif
}

bool Matcher::reload(const std::string& filename)
{
  hs_free_database(hs_db);
  hs_free_scratch(scratch);
  databases_from_file(filename.c_str(), &hs_db);
  if (hs_alloc_scratch(hs_db, &scratch) != HS_SUCCESS) {
    Util::eprint("failed to reload pattern file: ", filename);
    return false;
  }
  Util::iprint("success to reload pattern file: ", filename);
  return true;
}

int Matcher::on_match(unsigned int id, unsigned long long from,
                      unsigned long long to, unsigned int flags, void* ctx)
{
  // Our context points to a size_t storing the match count
  // size_t *matches = (size_t *)ctx;
  //(*matches)++;
  // return 0; // continue matching
  auto matches = (size_t*)ctx;
  (*matches)++;
  return 1;
}

Matcher::MatchResult Matcher::scan_block(const char* content,
                                         size_t content_length,
                                         const match_event_handler& onEvent)
{
  size_t matchCount = 0;
  hs_error_t err =
      hs_scan(hs_db, content, content_length, 0, scratch, onEvent, &matchCount);
  if (err == HS_SUCCESS || err == HS_SCAN_TERMINATED) {
    if (matchCount > 0) {
      return MatchResult::TRUE;
    }
  } else {
    Util::eprint("unable to scan packet: ", std::to_string(err));
    return MatchResult::ERROR;
  }
  return MatchResult::FALSE;
}

void Matcher::databases_from_file(const std::string& filename,
                                  hs_database_t** hs_db_ptr)
{
  // hs_compile_multi requires three parallel arrays containing the patterns,
  // flags and ids that we want to work with. To achieve this we use
  // std::vectors and new entries onto each for each valid line of input from
  // the pattern file.
  std::vector<std::string> patterns;
  std::vector<unsigned int> flags;
  std::vector<unsigned int> ids;

  // do the actual file reading and std::string handling
  parse_file(filename, patterns, flags, ids);

  // Turn our std::vector of std::strings into a std::vector of char*'s to pass
  // in to hs_compile_multi. (This is just using the std::vector of std::strings
  // as dynamic storage.)
  std::vector<const char*> cstr_patterns;
  cstr_patterns.reserve(patterns.size());
  for (const auto& pattern : patterns) {
    cstr_patterns.push_back(pattern.c_str());
  }

  Util::iprint("compiling hyperscan databases with patterns: ",
               std::to_string(patterns.size()));

  if (mode == Mode::BLOCK) {
    *hs_db_ptr = build_database(cstr_patterns, flags, ids, HS_MODE_BLOCK);
  } else {
    // TODO: Stream mode implementation
#if 0
    *hs_db_ptr = build_database(cstr_patterns, flags, ids, HS_MODE_STREAM);
#endif
  }
}

void Matcher::parse_file(const std::string& filename,
                         std::vector<std::string>& patterns,
                         std::vector<unsigned int>& flags,
                         std::vector<unsigned int>& ids)
{
  std::ifstream in_file(filename);
  if (!in_file.good()) {
    throw std::runtime_error("failed to open pattern file: " + filename);
  }

  for (unsigned int i = 1; !in_file.eof(); ++i) {
    std::string line;
    getline(in_file, line);

    if (line.empty() || line[0] == '#') {
      continue;
    }

#if 0
    //  10001:foobar/is
    size_t colon_idx = line.find_first_of(':');
    if (colon_idx == std::string::npos) {
      throw std::runtime_error("failed to parse pattern file: " + i);
    }

    unsigned int id = std::stoi(line.substr(0, colon_idx).c_str());

    const std::string expr(line.substr(colon_idx + 1));
    size_t flags_start = expr.find_last_of('/');
    if (flags_start == std::string::npos) {
      throw std::runtime_error("failed to parse pattern file('/' not found): " +
                               i);
    }

    std::string pcre(expr.substr(0, flags_start));
    std::string flags_str(expr.substr(flags_start + 1, expr.size() - flags_start));
#else
    std::string& pcre = line;
    std::string flags_str("");
#endif
    unsigned int flag = parse_flags(flags_str);

    patterns.push_back(pcre);
    flags.push_back(flag);
#if 0
    ids.push_back(id);
#else
    ids.push_back(i);
#endif
  }
}

hs_database_t*
Matcher::build_database(const std::vector<const char*>& expressions,
                        const std::vector<unsigned int>& flags,
                        const std::vector<unsigned int>& ids, unsigned int mode)
{
  hs_database_t* db;
  hs_compile_error_t* compile_err;
  hs_error_t err;

  err = hs_compile_multi(expressions.data(), flags.data(), ids.data(),
                         expressions.size(), mode, nullptr, &db, &compile_err);

  if (err != HS_SUCCESS) {
    if (compile_err->expression < 0) {
      // The error does not refer to a particular expression.
      Util::eprint("pattern compilation error: ", compile_err->message);
    } else {
      Util::eprint("pattern compilation error \'",
                   expressions[compile_err->expression],
                   "\': ", compile_err->message);
    }
    // As the compile_err pointer points to dynamically allocated memory, if
    // we get an error, we must be sure to release it. This is not
    // necessary when no error is detected.
    hs_free_compile_error(compile_err);
    throw std::runtime_error("failed to compile the pattern file");
  }

  return db;
}

unsigned int Matcher::parse_flags(const std::string& flags_str)
{
  unsigned int flags = 0;
  for (const auto& c : flags_str) {
    switch (c) {
    case 'i':
      flags |= HS_FLAG_CASELESS;
      break;
    case 'm':
      flags |= HS_FLAG_MULTILINE;
      break;
    case 's':
      flags |= HS_FLAG_DOTALL;
      break;
    case 'H':
      flags |= HS_FLAG_SINGLEMATCH;
      break;
    case 'V':
      flags |= HS_FLAG_ALLOWEMPTY;
      break;
    case '8':
      flags |= HS_FLAG_UTF8;
      break;
    case 'W':
      flags |= HS_FLAG_UCP;
      break;
    case '\r': // stray carriage-return
      break;
    default:
      throw std::runtime_error(std::string("unsupported flag '") + c + "'");
    }
  }
  return flags;
}
