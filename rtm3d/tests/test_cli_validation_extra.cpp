#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/cli/CliOptions.hpp"

TEST(CliOptionsExtra, RejectsZeroStride) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--receiver-stride", "0"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv)), std::runtime_error);
}

TEST(CliOptionsExtra, RejectsInvalidConfigOutputFormat) {
  std::filesystem::create_directories("tests/tmp_loader");
  {
    std::ofstream c("tests/tmp_loader/cfg_bad_format.json");
    c << "{\n"
      << "  \"data_dir\": \"data\",\n"
      << "  \"output_format\": \"bad\"\n"
      << "}\n";
  }
  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_bad_format.json"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv)), std::runtime_error);
}
