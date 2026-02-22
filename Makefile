CXX ?= g++
CXXFLAGS ?= -O2 -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
GTEST_DIR := third_party/googletest
GTEST_INC := -I$(GTEST_DIR)/googletest/include -I$(GTEST_DIR)/googletest

SRC = src/io/ArrayModelLoader.cpp src/io/GridModelLoader.cpp src/io/ImageIO.cpp src/rtm/RtmEngine.cpp src/rtm/Geometry.cpp src/rtm/Receivers.cpp src/rtm/Boundary.cpp src/rtm/Propagation.cpp src/rtm/Imaging.cpp src/rtm/Validation.cpp src/rtm/Acquisition.cpp src/rtm/Wavelet.cpp src/rtm/SourcePropagation.cpp src/rtm/ReceiverImaging.cpp src/rtm/ResultBuilder.cpp src/cli/CliOptions.cpp
TEST_SRC = tests/test_array_model_loader.cpp tests/test_array_loader_edge.cpp tests/test_cli_options.cpp tests/test_cli_validation_extra.cpp tests/test_rtm_engine.cpp tests/test_rtm_edge.cpp tests/test_rtm_acquisition.cpp tests/test_rtm_receivers.cpp tests/test_rtm_boundary.cpp tests/test_rtm_wavelet.cpp tests/test_image_io.cpp

all: build/rtm3d_cli build/rtm3d_tests

build:
	mkdir -p build

$(GTEST_DIR):
	mkdir -p third_party
	git clone --depth 1 --branch v1.14.0 https://github.com/google/googletest.git $(GTEST_DIR)

build/rtm3d_cli: build $(SRC) src/main.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/main.cpp -o $@

build/rtm3d_tests: build $(GTEST_DIR) $(SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(GTEST_INC) $(SRC) $(TEST_SRC) \
		$(GTEST_DIR)/googletest/src/gtest-all.cc $(GTEST_DIR)/googletest/src/gtest_main.cc \
		-pthread -o $@

test: build/rtm3d_tests
	./build/rtm3d_tests

e2e: build/rtm3d_cli
	bash tests/e2e_synthetic.sh

run: build/rtm3d_cli
	mkdir -p output
	./build/rtm3d_cli --data-dir data --output output/migrated_inline.pgm

static:
	@if command -v clang-tidy >/dev/null 2>&1; then \
		echo "[static] running clang-tidy"; \
		clang-tidy $$(find src -name '*.cpp' -type f | tr '\n' ' ') -- -std=c++20 -Iinclude; \
	elif command -v cppcheck >/dev/null 2>&1; then \
		echo "[static] running cppcheck"; \
		cppcheck --enable=warning,style,performance --std=c++20 --language=c++ -Iinclude src; \
	else \
		echo "[static] clang-tidy/cppcheck not found; running g++ -fanalyzer fallback"; \
		for f in src/io/*.cpp src/rtm/*.cpp src/cli/*.cpp src/main.cpp; do \
			g++ -std=c++20 -Wall -Wextra -Wpedantic -fanalyzer -Iinclude -fsyntax-only $$f; \
		done; \
	fi

clean:
	rm -rf build output/*.pgm output/*.png
