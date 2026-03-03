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

void expect_parse_throws_with_data_dir_config(const std::string& path,
                                              const std::string& key,
                                              const std::string& value) {
  write_config(path,
               "  \"data_dir\": \"data\",\n"
               "  \"" + key + "\": " + value + "\n");
  const char* argv[] = {"rtm3d_cli", "--config", path.c_str()};
  expect_parse_throws(argv);
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
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_shots.json", "n_shots", "0");
}

TEST(CliOptionsExtra, RejectsZeroReceiverStrideFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_receiver_stride.json", "receiver_stride", "0");
}

TEST(CliOptionsExtra, RejectsZeroDecimXFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_decim_x.json", "decim_x", "0");
}

TEST(CliOptionsExtra, RejectsZeroDecimZFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_decim_z.json", "decim_z", "0");
}

TEST(CliOptionsExtra, RejectsZeroPmlFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_pml.json", "pml", "0");
}

TEST(CliOptionsExtra, RejectsNyBelowMinimumFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_small_ny.json", "ny", "3");
}

TEST(CliOptionsExtra, RejectsNtBelowMinimumFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_small_nt.json", "nt", "1");
}

TEST(CliOptionsExtra, RejectsZeroDyFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_dy.json", "dy", "0.0");
}

TEST(CliOptionsExtra, RejectsZeroDtFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_dt.json", "dt", "0.0");
}

TEST(CliOptionsExtra, RejectsZeroF0FromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_zero_f0.json", "f0", "0.0");
}

TEST(CliOptionsExtra, RejectsNegativeDyFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_dy.json", "dy", "-0.1");
}

TEST(CliOptionsExtra, RejectsNegativeDtFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_dt.json", "dt", "-0.001");
}

TEST(CliOptionsExtra, RejectsNegativeF0FromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_f0.json", "f0", "-10.0");
}

TEST(CliOptionsExtra, RejectsNegativeReceiverStrideFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_receiver_stride.json", "receiver_stride", "-1");
}

TEST(CliOptionsExtra, RejectsNegativeDecimZFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_decim_z.json", "decim_z", "-1");
}

TEST(CliOptionsExtra, RejectsNegativeDecimXFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_decim_x.json", "decim_x", "-1");
}

TEST(CliOptionsExtra, RejectsNegativePmlFromConfig) {
  expect_parse_throws_with_data_dir_config(
      "tests/tmp_loader/cfg_negative_pml.json", "pml", "-1");
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

#define DEFINE_MISSING_VALUE_TEST(test_name, option) \
  TEST(CliOptionsExtra, test_name) { expect_missing_value_for(option); }

DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForOutputFormat, "--output-format")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForDy, "--dy")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForNt, "--nt")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForDecimX, "--decim-x")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForDecimZ, "--decim-z")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForCropX, "--crop-x")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForCropZ, "--crop-z")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForPml, "--pml")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForF0, "--f0")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForNy, "--ny")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForDt, "--dt")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForReceiverStride, "--receiver-stride")
DEFINE_MISSING_VALUE_TEST(RejectsMissingValueForNShots, "--n-shots")

#undef DEFINE_MISSING_VALUE_TEST

#define DEFINE_STANDALONE_MISSING_VALUE_TEST(test_name, option) \
  TEST(CliOptionsExtra, test_name) { expect_missing_value_for_standalone(option); }

DEFINE_STANDALONE_MISSING_VALUE_TEST(RejectsMissingValueForConfigPath, "--config")
DEFINE_STANDALONE_MISSING_VALUE_TEST(RejectsMissingValueForDataDir, "--data-dir")
DEFINE_STANDALONE_MISSING_VALUE_TEST(RejectsMissingValueForXFile, "--x-file")
DEFINE_STANDALONE_MISSING_VALUE_TEST(RejectsMissingValueForZFile, "--z-file")
DEFINE_STANDALONE_MISSING_VALUE_TEST(RejectsMissingValueForValuesFile, "--values-file")
DEFINE_STANDALONE_MISSING_VALUE_TEST(RejectsMissingValueForOutputPath, "--output")

#undef DEFINE_STANDALONE_MISSING_VALUE_TEST
