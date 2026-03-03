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

TEST(CliOptionsExtra, RejectsNonNumericFloatFromConfig) {
  write_config("tests/tmp_loader/cfg_bad_dy_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"dy\": \"oops\"\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_bad_dy_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsNonNumericSizeFromConfig) {
  write_config("tests/tmp_loader/cfg_bad_nt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"nt\": \"oops\"\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_bad_nt_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsNullNumericFromConfig) {
  write_config("tests/tmp_loader/cfg_null_dt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"dt\": null\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_null_dt_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsBooleanNumericFromConfig) {
  write_config("tests/tmp_loader/cfg_bool_nt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"nt\": true\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_bool_nt_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsArrayNumericFromConfig) {
  write_config("tests/tmp_loader/cfg_array_dt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"dt\": [0.001]\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_array_dt_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsObjectNumericFromConfig) {
  write_config("tests/tmp_loader/cfg_object_nt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"nt\": {\"value\": 100}\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_object_nt_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsQuotedFloatNumericFromConfig) {
  write_config("tests/tmp_loader/cfg_quoted_dt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"dt\": \"0.001\"\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_quoted_dt_token.json"};
  expect_parse_throws(argv);
}

TEST(CliOptionsExtra, RejectsQuotedSizeNumericFromConfig) {
  write_config("tests/tmp_loader/cfg_quoted_nt_token.json",
               "  \"data_dir\": \"data\",\n"
               "  \"nt\": \"100\"\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg_quoted_nt_token.json"};
  expect_parse_throws(argv);
}

#define DEFINE_CONFIG_REJECTION_TEST(test_name, path, key, value) \
  TEST(CliOptionsExtra, test_name) {                                     \
    expect_parse_throws_with_data_dir_config(path, key, value);          \
  }

DEFINE_CONFIG_REJECTION_TEST(RejectsZeroShotsFromConfig,
                             "tests/tmp_loader/cfg_zero_shots.json",
                             "n_shots", "0")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroReceiverStrideFromConfig,
                             "tests/tmp_loader/cfg_zero_receiver_stride.json",
                             "receiver_stride", "0")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroDecimXFromConfig,
                             "tests/tmp_loader/cfg_zero_decim_x.json",
                             "decim_x", "0")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroDecimZFromConfig,
                             "tests/tmp_loader/cfg_zero_decim_z.json",
                             "decim_z", "0")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroPmlFromConfig,
                             "tests/tmp_loader/cfg_zero_pml.json",
                             "pml", "0")
DEFINE_CONFIG_REJECTION_TEST(RejectsNyBelowMinimumFromConfig,
                             "tests/tmp_loader/cfg_small_ny.json",
                             "ny", "3")
DEFINE_CONFIG_REJECTION_TEST(RejectsNtBelowMinimumFromConfig,
                             "tests/tmp_loader/cfg_small_nt.json",
                             "nt", "1")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroDyFromConfig,
                             "tests/tmp_loader/cfg_zero_dy.json",
                             "dy", "0.0")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroDtFromConfig,
                             "tests/tmp_loader/cfg_zero_dt.json",
                             "dt", "0.0")
DEFINE_CONFIG_REJECTION_TEST(RejectsZeroF0FromConfig,
                             "tests/tmp_loader/cfg_zero_f0.json",
                             "f0", "0.0")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeDyFromConfig,
                             "tests/tmp_loader/cfg_negative_dy.json",
                             "dy", "-0.1")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeDtFromConfig,
                             "tests/tmp_loader/cfg_negative_dt.json",
                             "dt", "-0.001")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeF0FromConfig,
                             "tests/tmp_loader/cfg_negative_f0.json",
                             "f0", "-10.0")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeReceiverStrideFromConfig,
                             "tests/tmp_loader/cfg_negative_receiver_stride.json",
                             "receiver_stride", "-1")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeDecimZFromConfig,
                             "tests/tmp_loader/cfg_negative_decim_z.json",
                             "decim_z", "-1")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeDecimXFromConfig,
                             "tests/tmp_loader/cfg_negative_decim_x.json",
                             "decim_x", "-1")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativePmlFromConfig,
                             "tests/tmp_loader/cfg_negative_pml.json",
                             "pml", "-1")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeShotsFromConfig,
                             "tests/tmp_loader/cfg_negative_shots.json",
                             "n_shots", "-1")
DEFINE_CONFIG_REJECTION_TEST(RejectsNegativeNyFromConfig,
                             "tests/tmp_loader/cfg_negative_ny.json",
                             "ny", "-1")

#undef DEFINE_CONFIG_REJECTION_TEST

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
