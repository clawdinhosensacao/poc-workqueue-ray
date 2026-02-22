#pragma once

#include <string>

#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d {

enum class OutputFormat { kPgm8, kFloat32Raw };

struct CliOptions {
  std::string x_file;
  std::string z_file;
  std::string values_file;
  std::string output_file = "output/migrated_inline.pgm";
  OutputFormat output_format = OutputFormat::kPgm8;
  GridLoadOptions load;
  RtmConfig rtm;
  std::size_t n_shots = 1;  // 1 = single-shot (default), >1 = multi-shot with evenly spaced sources
};

CliOptions parse_cli_or_throw(int argc, char** argv);
std::string cli_help();

}  // namespace rtm3d
