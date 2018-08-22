#include <cstring>
#include <fstream>
#include <iostream>

#include "logConv.h"

using namespace std;

LogConv::LogConv(const std::string& filename)
{
  logfile.open(filename.c_str(), fstream::in);
  if (!logfile.is_open()) {
    throw runtime_error("Failed to open input file: " + filename);
  }
}
LogConv::LogConv(LogConv&& other) noexcept
{
  if (logfile.is_open()) {
    logfile.close();
  }
  logfile.swap(other.logfile);
}

LogConv::~LogConv()
{
  if (logfile.is_open()) {
    logfile.close();
  }
}

bool LogConv::skip(size_t count_skip)
{
  char buf[1];
  size_t count = 0;
  while (count < count_skip) {
    if (!logfile.getline(buf, 1)) {
      if (logfile.eof()) {
        return false;
      } else if (logfile.bad() || logfile.fail()) {
        return false;
      }
    }
    count++;
  }
  return true;
}

int LogConv::get_next_stream(char* message, size_t size)
{
  stream_length = 0;
  string line;
  if (!logfile.getline(message, size)) {
    if (logfile.eof()) {
      return RESULT_NO_MORE;
    } else if (logfile.bad() || logfile.fail()) {
      return RESULT_FAIL;
    }
  }
  stream_length = strlen(message);
  return stream_length;
}
