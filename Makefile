CXX ?= c++
SDL_CONFIG ?= sdl2-config
CPPFLAGS += $(shell $(SDL_CONFIG) --cflags)
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(SDL_CONFIG) --libs)
SOURCES := $(wildcard src/*.cpp src/app/*.cpp src/domain/*.cpp src/platform/*.cpp)

.PHONY: all check integration-check preview clean
all: build/hamclock-pro

build/hamclock-pro: $(SOURCES) $(wildcard src/*.h src/*.hpp src/app/*.hpp src/domain/*.hpp src/platform/*.hpp)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@ $(LDLIBS)

check: build/hamclock-pro
	./build/hamclock-pro --self-test

integration-check: build/hamclock-pro
	python3 tests/cli_regression.py ./build/hamclock-pro

preview: build/hamclock-pro
	./build/hamclock-pro --headless-frame build/dashboard.bmp

clean:
	rm -f build/hamclock-pro build/dashboard.bmp
