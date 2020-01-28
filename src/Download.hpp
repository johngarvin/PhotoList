#ifndef DOWNLOAD_HPP
#define DOWNLOAD_HPP

#include "PhotoData.hpp"

std::list<PhotoData> get_photo_data_from_json_url(std::string url, std::string aspect_ratio_string, int minimum_width);
SDL_Surface * load_jpeg_from_url(std::string url);

#endif
