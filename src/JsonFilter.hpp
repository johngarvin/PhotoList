#include <string>
#include <list>

#include "PhotoData.hpp"

#ifndef JSON_FILTER_HPP
#define JSON_FILTER_HPP

std::list<PhotoData> parse_and_filter(const char * json_string, std::string aspect_ratio, int minimum_width);

#endif
