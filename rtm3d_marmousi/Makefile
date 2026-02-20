CXX ?= g++
CXXFLAGS ?= -O3 -std=c++20 -Wall -Wextra -Wpedantic -Iinclude

SRC = src/MarmousiLoader.cpp src/RtmEngine.cpp src/ImageIO.cpp

all: build/rtm3d_cli build/rtm3d_tests

build:
	mkdir -p build

build/rtm3d_cli: build $(SRC) src/main.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/main.cpp -o $@

build/rtm3d_tests: build $(SRC) tests/test_main.cpp
	$(CXX) $(CXXFLAGS) $(SRC) tests/test_main.cpp -o $@

test: build/rtm3d_tests
	./build/rtm3d_tests

run: build/rtm3d_cli
	mkdir -p output
	./build/rtm3d_cli data output/migrated_inline.pgm

clean:
	rm -rf build output/*.pgm output/*.png
