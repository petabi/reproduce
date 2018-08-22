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
  virtual bool skip(size_t count_skip);
  virtual int get_next_stream(char* message, size_t size);

private:
  std::ifstream logfile;
};

#endif
