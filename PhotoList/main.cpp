//
//  main.cpp
//  PhotoList
//
//  Created by John Garvin on 2020-01-24.
//  Copyright © 2020 John Garvin. All rights reserved.
//

#include <stdio.h>
#include <vector>

#include "SDL.h"
#include "SDL_image.h"

class Uncopyable {
public:
    Uncopyable(const Uncopyable &) = delete;
    Uncopyable & operator=(const Uncopyable &) = delete;
protected:
    Uncopyable() = default;
    ~Uncopyable() = default;
};

void error(const char * message) {
    fprintf(stderr, "PhotoList error: %s", message);
    exit(1);
}

class PLView : Uncopyable {
private:
    SDL_Window * _window;
    SDL_Surface * _wsurface;
    std::vector<SDL_Surface *> _surfaces;
public:
    SDL_Window * getWindow() {
        return _window;
    }
    SDL_Surface * getSurface() {
        return _wsurface;
    }

    PLView() {
        if (SDL_Init( SDL_INIT_VIDEO ) < 0) {
            error("could not initialize with SDL_Init\n");
        }
        
        _window = SDL_CreateWindow("PhotoList",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   0,
                                   0,
                                   SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
        if (_window == NULL) {
            error("could not create window\n");
        }
        
        _wsurface = SDL_GetWindowSurface(_window);
        if (_wsurface == NULL) {
            error("could not create window surface\n");
        }
        int img_flags = IMG_INIT_JPG;
        int result = IMG_Init(img_flags);
        if (result != img_flags) {
            error("could not initialize SDL_image\n");
        }
    }

    ~PLView() {
        while (!_surfaces.empty()) {
            SDL_Surface * s = _surfaces.back();
            SDL_FreeSurface(s);
            _surfaces.pop_back();
        }
        IMG_Quit();
        SDL_FreeSurface(_wsurface);
        SDL_DestroyWindow(_window);
        SDL_Quit();
    }
    
    void show_bitmap(const char * file) {
        SDL_Surface * bitmap = SDL_LoadBMP(file);
        if (bitmap == NULL) {
            error("could not load bitmap\n");
        }
        _surfaces.push_back(bitmap);
        SDL_BlitSurface(bitmap, NULL, _wsurface, NULL);
        SDL_UpdateWindowSurface(_window);
    }
    
    void show_jpeg(const char * file) {
        SDL_Surface * jpeg = IMG_Load(file);
        if (jpeg == NULL) {
            error("could not load jpeg\n");
        }
        _surfaces.push_back(jpeg);
        SDL_BlitSurface(jpeg, NULL, _wsurface, NULL);
        SDL_UpdateWindowSurface(_window);
    }
};

class PLController : Uncopyable {
private:
    PLView _view;
    
public:
    PLController() : _view() {
    }
    
    void run() {
        //_view.show_bitmap("Zebra.bmp");
        _view.show_jpeg("1.jpg");
        
        // Run until quit
        SDL_Event event;
        bool is_running = true;
        while (is_running) {
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT) {
                    is_running = false;
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
