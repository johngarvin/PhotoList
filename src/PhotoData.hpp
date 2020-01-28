//
//  Photodata.hpp
//  PhotoList
//
//  Created by John Garvin on 2020-01-24.
//  Copyright © 2020 John Garvin. All rights reserved.
//

#ifndef PHOTO_DATA_HPP
#define PHOTO_DATA_HPP

// Representation of one photo.
struct PhotoData {
  std::string headline;
  std::string subhead;
  std::string url;
  int width;
  int height;
};

#endif
