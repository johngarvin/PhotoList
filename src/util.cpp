#include <iostream>

#include "util.hpp"

void error(const char * message) {
  std::cerr << "PhotoList error: " << message << std::endl;
  exit(1);
}
