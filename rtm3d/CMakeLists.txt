cmake_minimum_required(VERSION 3.20)
project(rtm3d VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RTM3D_BUILD_TESTS "Build tests" ON)

add_library(rtm3d
    src/io/ArrayModelLoader.cpp
    src/io/GridModelLoader.cpp
    src/io/ImageIO.cpp
    src/rtm/RtmEngine.cpp
    src/cli/CliOptions.cpp
)

target_include_directories(rtm3d PUBLIC include)
target_compile_options(rtm3d PRIVATE -Wall -Wextra -Wpedantic)

add_executable(rtm3d_cli src/main.cpp)
target_link_libraries(rtm3d_cli PRIVATE rtm3d)

if(RTM3D_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(rtm3d_tests
    tests/test_array_model_loader.cpp
    tests/test_cli_options.cpp
    tests/test_rtm_engine.cpp
    tests/test_image_io.cpp
  )
  target_link_libraries(rtm3d_tests PRIVATE rtm3d GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(rtm3d_tests)
endif()
