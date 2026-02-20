cmake_minimum_required(VERSION 3.20)
project(rtm3d VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RTM3D_BUILD_TESTS "Build tests" ON)

add_library(rtm3d
    src/MarmousiLoader.cpp
    src/RtmEngine.cpp
    src/ImageIO.cpp
)
target_include_directories(rtm3d PUBLIC include)
target_compile_options(rtm3d PRIVATE -Wall -Wextra -Wpedantic)

add_executable(rtm3d_cli src/main.cpp)
target_link_libraries(rtm3d_cli PRIVATE rtm3d)

if(RTM3D_BUILD_TESTS)
  enable_testing()
  add_executable(rtm3d_tests tests/test_main.cpp)
  target_link_libraries(rtm3d_tests PRIVATE rtm3d)
  add_test(NAME rtm3d_tests COMMAND rtm3d_tests)
endif()
