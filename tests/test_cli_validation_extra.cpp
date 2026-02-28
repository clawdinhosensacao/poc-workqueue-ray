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

TEST(CliOptionsExtra, RejectsNegativeUnsignedOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--decim-x", "-1"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv)), std::runtime_error);
}

TEST(CliOptionsExtra, ConfigOutputAliasOverridesOutputFile) {
  std::filesystem::create_directories("tests/tmp_loader");
  {
    std::ofstream c("tests/tmp_loader/cfg_output_alias.json");
    c << "{\n"
      << "  \"data_dir\": \"data\",\n"
      << "  \"output_file\": \"output/from_output_file.pgm\",\n"
      << "  \"output\": \"output/from_output_alias.pgm\"\n"
      << "}\n";
  }

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_output_alias.json"};
  const auto o = rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv));
  EXPECT_EQ(o.output_file, "output/from_output_alias.pgm");
}

TEST(CliOptionsExtra, RejectsInvalidCliOutputFormat) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--output-format", "bad"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}

TEST(CliOptionsExtra, RejectsInvalidCliFloatOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy", "bad"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}

TEST(CliOptionsExtra, RejectsOutOfRangeCliFloatOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy", "1e999"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}

TEST(CliOptionsExtra, RejectsOutOfRangeCliSizeOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--nt", "18446744073709551616"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}

TEST(CliOptionsExtra, RejectsMissingValueForOutputFormat) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--output-format"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}

TEST(CliOptionsExtra, RejectsMissingValueForDy) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}

TEST(CliOptionsExtra, RejectsMissingValueForNt) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--nt"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                               const_cast<char**>(argv)),
               std::runtime_error);
}
