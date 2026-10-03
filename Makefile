CXX ?= c++
SDL_CONFIG ?= sdl2-config
CPPFLAGS += $(shell $(SDL_CONFIG) --cflags)
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(SDL_CONFIG) --libs)
SOURCES := $(wildcard src/*.cpp src/app/*.cpp src/domain/*.cpp src/platform/*.cpp src/map/*.cpp)

.PHONY: all check integration-check map-check locator-check verify preview clean
all: build/hamclock-pro

build/hamclock-pro: $(SOURCES) $(wildcard src/*.h src/*.hpp src/app/*.hpp src/domain/*.hpp src/platform/*.hpp src/map/*.hpp)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@ $(LDLIBS)

check: build/hamclock-pro
	./build/hamclock-pro --self-test

integration-check: build/hamclock-pro
	python3 tests/cli_regression.py ./build/hamclock-pro

map-check:
	python3 tests/map_import_regression.py

build/locator-input-regression: tests/locator_input_regression.cpp src/domain/locator_input.cpp src/domain/locator_input.hpp src/domain/geometry.cpp src/domain/geometry.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) tests/locator_input_regression.cpp src/domain/locator_input.cpp src/domain/geometry.cpp -o $@

build/locator-boundary-regression: tests/locator_boundary_regression.cpp src/domain/locator_input.cpp src/domain/locator_input.hpp src/domain/geometry.cpp src/domain/geometry.hpp src/domain/tests.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) tests/locator_boundary_regression.cpp src/domain/locator_input.cpp src/domain/geometry.cpp src/domain/tests.cpp -o $@

locator-check: build/locator-input-regression build/locator-boundary-regression
	./build/locator-input-regression
	./build/locator-boundary-regression

verify: check integration-check map-check locator-check

preview: build/hamclock-pro
	./build/hamclock-pro --headless-frame build/dashboard.bmp

clean:
	rm -f build/hamclock-pro build/dashboard.bmp
