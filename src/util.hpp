#ifndef UTIL_HPP
#define UTIL_HPP

class Uncopyable {
public:
  Uncopyable(const Uncopyable &) = delete;
  Uncopyable & operator=(const Uncopyable &) = delete;
protected:
  Uncopyable() = default;
  ~Uncopyable() = default;
};

void error(const char * message);

#endif
