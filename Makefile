.PHONY: all
all: PhotoList

ifeq ($(CURDIR),)
CURDIR := $(shell pwd)
endif

CC := clang++

LIBS := -Lexternal-libs-install/SDL2-install/lib -lSDL2 -Lexternal-libs-install/SDL2_image-install/lib -lSDL2_image -Lexternal-libs-install/SDL2_ttf-install/lib -lSDL2_ttf -Lexternal-libs-install/curl-install/lib -lcurl
INCLUDES := -Iexternal-src/json11-master -Iexternal-libs-install/SDL2-install/include/SDL2 -Iexternal-libs-install/SDL2_image-install/include/SDL2 -Iexternal-libs-install/SDL2_ttf-install/include/SDL2 -Iexternal-libs-install/curl-install/include/curl

external-libs-install/SDL2-install/lib/libSDL2.dylib:
	cd external-libs/SDL2-2.0.10 && ./configure --prefix=$(CURDIR)/external-libs-install/SDL2-install && $(MAKE) -j4 && $(MAKE) install

external-libs-install/SDL2_image-install/lib/libSDL2_image.dylib: external-libs-install/SDL2-install/lib/libSDL2.dylib
	cd external-libs/SDL2_image-2.0.5 && ./configure --prefix=$(CURDIR)/external-libs-install/SDL2_image-install --with-sdl-prefix=$(CURDIR)/external-libs-install/SDL2-install && $(MAKE) -j4 && $(MAKE) install

external-libs-install/SDL2_ttf-install/lib/libSDL2_ttf.dylib: external-libs-install/SDL2-install/lib/libSDL2.dylib
	cd external-libs/SDL2_ttf-2.0.15 && ./configure --prefix=$(CURDIR)/external-libs-install/SDL2_ttf-install --with-sdl-prefix=$(CURDIR)/external-libs-install/SDL2-install && $(MAKE) -j4 && $(MAKE) install

external-libs-install/curl-install/lib/libcurl.dylib:
	cd external-libs/curl-7.68.0 && ./configure --prefix=$(CURDIR)/external-libs-install/curl-install && $(MAKE) -j4 && $(MAKE) install

PhotoList: Makefile src/main.cpp src/JsonFilter.cpp src/JsonFilter.hpp src/PhotoData.hpp src/util.cpp src/util.hpp src/Download.cpp src/Download.hpp external-src/json11-master/json11.cpp external-src/json11-master/json11.hpp external-libs-install/SDL2-install/lib/libSDL2.dylib external-libs-install/SDL2_image-install/lib/libSDL2_image.dylib external-libs-install/SDL2_ttf-install/lib/libSDL2_ttf.dylib external-libs-install/curl-install/lib/libcurl.dylib
	$(CC) -o PhotoList -O3 -g -fsanitize=undefined -fsanitize=address -std=c++11 src/main.cpp src/JsonFilter.cpp src/util.cpp src/Download.cpp external-src/json11-master/json11.cpp $(LIBS) $(INCLUDES)

.PHONY: clean
clean:
	cd $(CURDIR)/external-libs/SDL2-2.0.10 && make clean
	cd $(CURDIR)/external-libs/SDL2_image-2.0.5 && make clean
	cd $(CURDIR)/external-libs/curl-7.68.0 && make clean
	rm -rf $(CURDIR)/external-libs-install/SDL2-install $(CURDIR)/external-libs-install/SDL2_image-install PhotoList
