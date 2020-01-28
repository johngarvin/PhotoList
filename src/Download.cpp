#include <list>
#include <string>

#include "SDL.h"
#include "SDL_image.h"
#include "curl.h"

#include "Download.hpp"
#include "JsonFilter.hpp"
#include "util.hpp"

struct MemoryStruct {
  size_t size;
  char * memory;
};

static SDL_Surface * load_jpeg_from_mem(MemoryStruct ms) {
  SDL_RWops * source = SDL_RWFromMem(ms.memory, ms.size);
  if (source == nullptr) {
    error("Couldn't get SDL_RWops");
  }
  SDL_Surface * jpeg = IMG_Load_RW(source, 1 /* free source after use */);
  if (jpeg == nullptr) {
    error("Couldn't load jpeg from memory");
  }
  return jpeg;
}

static size_t write_memory_callback(void * contents,
                                    size_t size,
                                    size_t nmemb,
                                    void * userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char * ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == nullptr) {
    error("Ran out of memory while downloading data");
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

// Fetch data from the given url. Allocates memory in the memory field of the
// MemoryStruct returned.
static MemoryStruct fetch(std::string url) {
  struct MemoryStruct chunk;
  chunk.size = 0;                    /* no data yet */ 
  chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
 
  curl_global_init(CURL_GLOBAL_ALL);
  CURL * curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode result = curl_easy_perform(curl_handle);
  if(result != CURLE_OK) {
    error("curl failed");
  }

  return chunk;
}

// Creates a list of PhotoData.
std::list<PhotoData> get_photo_data_from_json_url(std::string url,
                                                  std::string aspect_ratio_string,
                                                  int minimum_width) {
  MemoryStruct json = fetch(url);
  std::list<PhotoData> data = parse_and_filter(json.memory,
                                               aspect_ratio_string,
                                               minimum_width);
  free(json.memory);
  return data;
}

SDL_Surface * load_jpeg_from_url(std::string url) {
  return load_jpeg_from_mem(fetch(url));
}
