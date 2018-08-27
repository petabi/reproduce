#ifndef LOGCONV_H
#define LOGCONV_H

#include <fstream>

#include "conv.h"

class LogConv : public Conv {
public:
  LogConv() = delete;
  LogConv(const std::string& filename);
  LogConv(const LogConv&) = delete;
  LogConv& operator=(const LogConv&) = delete;
  LogConv(LogConv&& other) noexcept;
  LogConv& operator=(const LogConv&&) = delete;
  ~LogConv();
  bool skip(size_t count_skip) override;
  int get_next_stream(char* message, size_t size) override;

private:
  std::ifstream logfile;
};

#endif

// vim: et:ts=2:sw=2
