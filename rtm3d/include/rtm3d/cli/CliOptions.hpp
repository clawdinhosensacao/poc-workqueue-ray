#pragma once

#include <string>

#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d {

struct CliOptions {
  std::string x_file;
  std::string z_file;
  std::string values_file;
  std::string output_file = "output/migrated_inline.pgm";
  GridLoadOptions load;
  RtmConfig rtm;
};

CliOptions parse_cli_or_throw(int argc, char** argv);
std::string cli_help();

}  // namespace rtm3d
