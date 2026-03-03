#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/cli/CliOptions.hpp"

namespace {

constexpr const char* kTmpLoaderDir = "tests/tmp_loader";

template <std::size_t N>
void expect_parse_throws(const char* (&argv)[N]) {
  EXPECT_THROW(
      (void)rtm3d::parse_cli_or_throw(static_cast<int>(N), const_cast<char**>(argv)),
      std::runtime_error);
}

void write_config(const std::string& path, const std::string& json_body) {
  std::filesystem::create_directories(kTmpLoaderDir);
  std::ofstream c(path);
  c << "{\n"
    << json_body
    << "}\n";
}

}  // namespace

TEST(CliOptionsExtra, RejectsZeroStride) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--receiver-stride", "0"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsZeroShots) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--n-shots", "0"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsInvalidConfigOutputFormat) {
  write_config("tests/tmp_loader/cfg_bad_format.json",
               "  \"data_dir\": \"data\",\n"
               "  \"output_format\": \"bad\"\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_bad_format.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsZeroShotsFromConfig) {
  write_config("tests/tmp_loader/cfg_zero_shots.json",
               "  \"data_dir\": \"data\",\n"
               "  \"n_shots\": 0\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_zero_shots.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsNegativeUnsignedOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--decim-x", "-1"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, ConfigOutputAliasOverridesOutputFile) {
  write_config("tests/tmp_loader/cfg_output_alias.json",
               "  \"data_dir\": \"data\",\n"
               "  \"output_file\": \"output/from_output_file.pgm\",\n"
               "  \"output\": \"output/from_output_alias.pgm\"\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_output_alias.json"};
  const auto o = rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                            const_cast<char**>(argv));
  EXPECT_EQ(o.output_file, "output/from_output_alias.pgm");
}

TEST(CliOptionsExtra, RejectsInvalidCliOutputFormat) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--output-format", "bad"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsInvalidCliFloatOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy", "bad"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsOutOfRangeCliFloatOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy", "1e999"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsNonFiniteCliFloatOptionInf) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy", "inf"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsNonFiniteCliFloatOptionNaN) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy", "nan"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsOutOfRangeCliSizeOption) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--nt", "18446744073709551616"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForOutputFormat) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--output-format"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForDy) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--dy"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForNt) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--nt"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForNShots) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--n-shots"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForConfigPath) {
  const char* argv[] = {"rtm3d_cli", "--config"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForDataDir) {
  const char* argv[] = {"rtm3d_cli", "--data-dir"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForXFile) {
  const char* argv[] = {"rtm3d_cli", "--x-file"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForZFile) {
  const char* argv[] = {"rtm3d_cli", "--z-file"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForValuesFile) {
  const char* argv[] = {"rtm3d_cli", "--values-file"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsMissingValueForOutputPath) {
  const char* argv[] = {"rtm3d_cli", "--output"};
  expect_parse_throws(argv);
}
