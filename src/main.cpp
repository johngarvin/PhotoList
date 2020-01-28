//
//  main.cpp
//  PhotoList
//
//  Created by John Garvin on 2020-01-24.
//  Copyright © 2020 John Garvin. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "curl.h"

#include "JsonFilter.hpp"
#include "PhotoData.hpp"
#include "util.hpp"

const char * json_url = "http://statsapi.mlb.com/api/v1/schedule?hydrate=game(content(editorial(recap))),decisions&date=2018-06-10&sportId=1";

// Choose the smallest photos at least this width in pixels
const int minimum_width = 400;

// Set aspect ratio here
static const char * aspect_ratio_string = "16:9";
static int box_height_for_width(int width) {
  return width * 9 / 16;
}

// Set scale factor of focused box here
static int scale_fbox(int x) {
  return x * 3 / 2;
}

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

static std::list<PhotoData> get_photo_data_from_json_url(std::string url) {
  MemoryStruct json = fetch(url);
  std::list<PhotoData> data = parse_and_filter(json.memory, aspect_ratio_string, 400);
  free(json.memory);
  return data;
}

static SDL_Surface * load_jpeg_from_url(std::string url) {
  return load_jpeg_from_mem(fetch(url));
}

class PLView : Uncopyable {
private:
  SDL_Window * _window;
  SDL_Surface * _wsurface;

  // height and width of the screen
  int _height;
  int _width;

  SDL_Surface * _background;

  std::list<SDL_Surface *> _boxes;
  std::list<SDL_Surface *>::iterator _fbox;  // box that is focused
  
  const int _box_w = 300;      // size of non-focused box
  const int _box_h = box_height_for_width(_box_w);
  const int _box_spacing = 100;  // distance between boxes
  int _box_middle_y;             // y coordinate of middle of non-focused boxes
  int _box_y;
  int _fbox_x;
  int _fbox_y;
  int _fbox_w;
  int _fbox_h;

  void render_background() {
    int result = SDL_BlitScaled(_background, NULL, _wsurface, NULL);
    if (result != 0) {
      error("render_background: couldn't blit background");
    }
    SDL_UpdateWindowSurface(_window);
  }
  
  void render_surface(SDL_Surface * box, int x, int y, int w, int h) {
    SDL_Rect dstrect;
    dstrect.x = x;
    dstrect.y = y;
    dstrect.w = w;
    dstrect.h = h;
    int result = SDL_BlitScaled(box, NULL, _wsurface, &dstrect);
    if (result != 0) {
      error("render_surface: couldn't blit surface");
    }
  }

public:
  const int n_displayed_each_side = 3;  // # boxes left or right of fbox

  PLView() {
    int result;
    
    result = SDL_Init(SDL_INIT_VIDEO);
    if (result != 0) {
      error("could not initialize SDL");
    }

    _window = SDL_CreateWindow("PhotoList",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               0,
                               0,
                               SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
    if (_window == nullptr) {
      error("could not create window");
    }

    _wsurface = SDL_GetWindowSurface(_window);
    if (_wsurface == nullptr) {
      error("could not create window surface");
    }

    SDL_DisplayMode dm;
    result = SDL_GetCurrentDisplayMode(0, &dm);
    if (result != 0) {
      error("Could not get current display mode");
    }

    _height = dm.h;
    _width = dm.w;
    _box_middle_y = _height * 2 / 5;
    _box_y = _box_middle_y - _box_h / 2;
    _fbox_x = _width / 2 - scale_fbox(_box_w) / 2;
    _fbox_y = _box_middle_y - scale_fbox(_box_h) / 2;
    _fbox_w = scale_fbox(_box_w);
    _fbox_h = scale_fbox(_box_h);
  }

  ~PLView() {
    while (!_boxes.empty()) {
      SDL_Surface * s = _boxes.back();
      SDL_FreeSurface(s);
      _boxes.pop_back();
    }
    if (_background != nullptr) {
      SDL_FreeSurface(_background);
    }
    SDL_FreeSurface(_wsurface);
    SDL_DestroyWindow(_window);
    SDL_Quit();
  }
  
  void set_background(SDL_Surface * background) {
    _background = background;
    render_background();
  }

  void render_all(const std::list<SDL_Surface *> & left_boxes,
                 const std::list<SDL_Surface *> & right_boxes,
                 SDL_Surface * fbox,
                 SDL_Surface * headline,
                 SDL_Surface * subhead) {
    render_background();

    // render headline
    if (headline != nullptr) {
      render_surface(headline,
                    _width / 2 - headline->w / 2,
                    _box_middle_y - _fbox_h / 2 - _box_spacing,
                    headline->w,
                    headline->h);
    }

    // render subhead
    if (subhead != nullptr) {
      render_surface(subhead,
                    _width / 2 - subhead->w / 2,
                    _box_middle_y + _fbox_h / 2 + _box_spacing,
                    subhead->w,
                    subhead->h);
    }
    
    // render fbox
    render_surface(fbox, _fbox_x, _fbox_y, _fbox_w, _fbox_h);

    // render left boxes
    int x = _fbox_x - _box_spacing - _box_w;
    for (auto b : left_boxes) {
      render_surface(b, x, _box_y, _box_w, _box_h);
      x -= _box_w + _box_spacing;
    }
    
    // render right boxes
    x = _fbox_x + _fbox_w + _box_spacing;
    for (auto b : right_boxes) {
      render_surface(b, x, _box_y, _box_w, _box_h);
      x += _box_w + _box_spacing;
    }

    SDL_UpdateWindowSurface(_window);
  }
};

class PLViewWrapper : Uncopyable {
private:
  const char * _background_filename = "images/1.jpg";

  std::list<PhotoData> _games;
  std::list<PhotoData>::iterator _fgame;  // box that is focused
  std::list<PhotoData>::iterator _begin_displayed;
  std::list<PhotoData>::iterator _end_displayed;
  
  // Surfaces
  SDL_Surface * _fbox_surface;
  std::list<SDL_Surface *> _left_surfaces;
  std::list<SDL_Surface *> _right_surfaces;
  int _left_size;  // size of _left_surfaces
  int _right_size; // size of _right_surfaces
  
  const char * _font_filename = "fonts/LiberationSans-Regular.ttf";
  TTF_Font * _headline_font;
  TTF_Font * _subhead_font;
  const int _headline_font_size = 48;
  const int _subhead_font_size = 24;
  
  PLView _view;

public:
  PLViewWrapper() : _view(),
                    _games(get_photo_data_from_json_url(json_url)),
                    _fgame(_games.begin()),
                    _begin_displayed(_fgame),
                    _end_displayed(_fgame) {
    int result;

    int img_flags = IMG_INIT_JPG;
    result = IMG_Init(img_flags);
    if (result != img_flags) {
      error("could not initialize SDL_image");
    }

    result = TTF_Init();
    if (result != 0) {
      error("Couldn't initialize TTF");
    }
    _headline_font = TTF_OpenFont(_font_filename, _headline_font_size);
    if (_headline_font == nullptr) {
      error("Couldn't open font");
    }
    _subhead_font = TTF_OpenFont(_font_filename, _subhead_font_size);
    if (_subhead_font == nullptr) {
      error("Couldn't open font");
    }

    SDL_Surface * background = IMG_Load(_background_filename);
    if (background == NULL) {
      error("could not load background");
    }
    _view.set_background(background);
  }

  ~PLViewWrapper() {
    while (!_games.empty()) {
      _games.pop_back();
    }
    IMG_Quit();
    TTF_CloseFont(_headline_font);
    TTF_CloseFont(_subhead_font);
    TTF_Quit();
    SDL_FreeSurface(_fbox_surface);
    for (auto s : _left_surfaces) {
      SDL_FreeSurface(s);
    }
    for (auto s : _right_surfaces) {
      SDL_FreeSurface(s);
    }
  }

  void initialize_surfaces() {
    _left_size = 0;
    _right_size = 0;

    _fbox_surface = load_jpeg_from_url(_fgame->url);

    // Create displayed left boxes in right to left order
    _begin_displayed = _fgame;
    for (int i = 0; i < _view.n_displayed_each_side; i++) {
      if (_begin_displayed == _games.begin()) {
        break;
      }
      _begin_displayed--;
      _left_surfaces.push_back(load_jpeg_from_url(_begin_displayed->url));
      _left_size++;
    }

    // Create displayed right boxes in left to right order
    _end_displayed = _fgame;
    _end_displayed++;
    for (int i = 0; i < _view.n_displayed_each_side; i++) {
      if (_end_displayed == _games.end()) {
        break;
      }
      _right_surfaces.push_back(load_jpeg_from_url(_end_displayed->url));
      _right_size++;
      _end_displayed++;
    }
  }
  
  void render_all() {
    // Create headline and subhead
    const SDL_Color white = {255, 255, 255, 255};
    SDL_Surface * headline;
    if (_fgame->headline.empty()) {
      headline = nullptr;
    } else {
      headline = TTF_RenderUTF8_Solid(_headline_font,
                                      _fgame->headline.c_str(),
                                      white);
    }
    SDL_Surface * subhead;
    if (_fgame->subhead.empty()) {
      subhead = nullptr;
    } else {
      subhead = TTF_RenderUTF8_Solid(_subhead_font,
                                     _fgame->subhead.c_str(),
                                     white);
    }
    
    _view.render_all(_left_surfaces,
                    _right_surfaces,
                    _fbox_surface,
                    headline,
                    subhead);
    SDL_FreeSurface(headline);
    SDL_FreeSurface(subhead);
  }

  void move_right() {
    _fgame++;
    if (_fgame == _games.end()) {
      _fgame--;
    } else {
      // remove leftmost if left is full
      if (_left_size == _view.n_displayed_each_side) {
        SDL_FreeSurface(_left_surfaces.back());
        _begin_displayed++;
        _left_surfaces.pop_back();
        _left_size--;
      }
      // move fbox to left
      _left_surfaces.push_front(_fbox_surface);
      _left_size++;

      // right of fbox is the new fbox
      _fbox_surface = _right_surfaces.front();
      _right_surfaces.pop_front();
      _right_size--;

      // if there's a new rightmost, grab it
      if (_end_displayed != _games.end()) {
        _right_surfaces.push_back(load_jpeg_from_url(_end_displayed->url));
        _right_size++;
        _end_displayed++;
      }
      
      render_all();
    }
#if 0
    // debug
    std::cout << "After move right: left surface " << _left_surfaces.size() << "=" << _left_size << std::endl;
    std::cout << "right surface " << _right_surfaces.size() << "=" << _right_size << std::endl;
#endif
  }

  void move_left() {
    if (_fgame != _games.begin()) {
      _fgame--;

      // remove rightmost if right is full
      if (_right_size == _view.n_displayed_each_side) {
        SDL_FreeSurface(_right_surfaces.back());
        _end_displayed--;
        _right_surfaces.pop_back();
        _right_size--;
      }

      // move fbox to right
      _right_surfaces.push_front(_fbox_surface);
      _right_size++;

      // left of fbox is the new fbox
      _fbox_surface = _left_surfaces.front();
      _left_surfaces.pop_front();
      _left_size--;

      // if there's a new leftmost, grab it
      if (_begin_displayed != _games.begin()) {
        _begin_displayed--;
        _left_surfaces.push_back(load_jpeg_from_url(_begin_displayed->url));
        _left_size++;
      }
      render_all();
    }
#if 0
    // debug
    std::cout << "After move left: left surfaces " << _left_surfaces.size() << "=" << _left_size << std::endl;
    std::cout << "right surfaces " << _right_surfaces.size() << "=" << _right_size << std::endl;
#endif
  }

};

class PLController : Uncopyable {
private:
  PLViewWrapper _view_wrapper;

public:
  PLController() : _view_wrapper() {
  }

  void run() {
    _view_wrapper.initialize_surfaces();
    _view_wrapper.render_all();
    
    SDL_Event event;
    bool is_running = true;
    while (is_running) {
      while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
          is_running = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
          case SDLK_LEFT:
            _view_wrapper.move_left();
            break;
          case SDLK_RIGHT:
            _view_wrapper.move_right();
            break;
          case SDLK_q:
            is_running = false;
            break;
          }
          break;
        }
      }
      SDL_Delay(16);
    }
  }
};

int main(int argc, const char * argv[]) {
  PLController c;
  c.run();
  return 0;
}
