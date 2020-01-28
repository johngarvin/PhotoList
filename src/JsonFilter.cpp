#include <iostream>

#include "json11.hpp"

#include "JsonFilter.hpp"
#include "PhotoData.hpp"
#include "util.hpp"

static json11::Json parse_json(const char * json_string) {
  std::string err;
  json11::Json json = json11::Json::parse(json_string, err);
  if (!err.empty()) {
    std::cerr << err;
    error("could not parse json");
  }
  return json;
}

// From the given JSON, select a sequence of photos. Each photo generally comes
// in multiple aspect ratios and sizes. Of the photos with the given aspect
// ratio, grab the photo with the smallest width that is at least minimum_width.
static std::list<PhotoData> filter(const json11::Json json,
                                   std::string aspect_ratio_string,
                                   int minimum_width) {
  std::list<PhotoData> data;

  json = json["dates"][0]["games"];
  for (json11::Json i : json.array_items()) {
    PhotoData pd;
    json11::Json mlb = i["content"]["editorial"]["recap"]["mlb"];
    pd.headline = mlb["headline"].string_value();
    pd.subhead = mlb["subhead"].string_value();
    int best_width = INT_MAX;
    int best_height = INT_MAX;
    std::string best_url;
    for (json11::Json j : mlb["image"]["cuts"].array_items()) {
      std::string url;
      if (j["aspectRatio"].string_value() != aspect_ratio_string) {
        continue;
      }
      int width = j["width"].int_value();
      if (width < minimum_width) {
        continue;
      }
      if (width < best_width) {
        best_width = width;
        best_height = j["height"].int_value();
        best_url = j["src"].string_value();
      }
    }
    if (best_width == INT_MAX) {
      error("couldn't find photo with width at least minimum_width");
    }
    pd.height = best_height;
    pd.width = best_width;
    pd.url = best_url;
    data.push_back(pd);
  }
  return data;
}

std::list<PhotoData> parse_and_filter(const char * json_string, std::string aspect_ratio, int minimum_width) {
  return filter(parse_json(json_string), aspect_ratio, minimum_width);
}
