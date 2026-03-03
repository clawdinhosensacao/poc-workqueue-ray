#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/cli/CliOptions.hpp"

namespace {

constexpr const char* kTmpLoaderDir = "tests/tmp_loader";

void expect_parse_throws(int argc, const char* const* argv) {
  EXPECT_THROW(
      (void)rtm3d::parse_cli_or_throw(argc, const_cast<char**>(argv)),
      std::runtime_error);
}

template <std::size_t N>
void expect_parse_throws(const char* (&argv)[N]) {
  expect_parse_throws(static_cast<int>(N), argv);
}

void expect_missing_value_for(const char* option) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", option};
  expect_parse_throws(argv);
}

void expect_missing_value_for_standalone(const char* option) {
  const char* argv[] = {"rtm3d_cli", option};
  expect_parse_throws(argv);
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
  expect_missing_value_for("--output-format");
}

TEST(CliOptionsExtra, RejectsMissingValueForDy) {
  expect_missing_value_for("--dy");
}

TEST(CliOptionsExtra, RejectsMissingValueForNt) {
  expect_missing_value_for("--nt");
}

TEST(CliOptionsExtra, RejectsMissingValueForDecimX) {
  expect_missing_value_for("--decim-x");
}

TEST(CliOptionsExtra, RejectsMissingValueForDecimZ) {
  expect_missing_value_for("--decim-z");
}

TEST(CliOptionsExtra, RejectsMissingValueForCropX) {
  expect_missing_value_for("--crop-x");
}

TEST(CliOptionsExtra, RejectsMissingValueForCropZ) {
  expect_missing_value_for("--crop-z");
}

TEST(CliOptionsExtra, RejectsMissingValueForPml) {
  expect_missing_value_for("--pml");
}

TEST(CliOptionsExtra, RejectsMissingValueForF0) {
  expect_missing_value_for("--f0");
}

TEST(CliOptionsExtra, RejectsMissingValueForNy) {
  expect_missing_value_for("--ny");
}

TEST(CliOptionsExtra, RejectsMissingValueForDt) {
  expect_missing_value_for("--dt");
}

TEST(CliOptionsExtra, RejectsMissingValueForReceiverStride) {
  expect_missing_value_for("--receiver-stride");
}

TEST(CliOptionsExtra, RejectsMissingValueForNShots) {
  expect_missing_value_for("--n-shots");
}

TEST(CliOptionsExtra, RejectsMissingValueForConfigPath) {
  expect_missing_value_for_standalone("--config");
}

TEST(CliOptionsExtra, RejectsMissingValueForDataDir) {
  expect_missing_value_for_standalone("--data-dir");
}

TEST(CliOptionsExtra, RejectsMissingValueForXFile) {
  expect_missing_value_for_standalone("--x-file");
}

TEST(CliOptionsExtra, RejectsMissingValueForZFile) {
  expect_missing_value_for_standalone("--z-file");
}

TEST(CliOptionsExtra, RejectsMissingValueForValuesFile) {
  expect_missing_value_for_standalone("--values-file");
}

TEST(CliOptionsExtra, RejectsMissingValueForOutputPath) {
  expect_missing_value_for_standalone("--output");
}
