#ifndef CONV_H
#define CONV_H

enum class ConvResult { FAIL = -2, NO_MORE = -1 };

class Conv {
public:
  virtual bool skip(size_t size) = 0;
  virtual int get_next_stream(char* message, size_t size) = 0;
  int stream_length = 0;

private:
};

#endif
