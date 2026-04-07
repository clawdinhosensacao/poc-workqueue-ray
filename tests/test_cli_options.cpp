#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/cli/CliOptions.hpp"

namespace {

template <std::size_t N>
rtm3d::CliOptions parse_cli(const char* (&argv)[N]) {
  return rtm3d::parse_cli_or_throw(static_cast<int>(N), const_cast<char**>(argv));
}

void write_config_file(const std::string& path, const std::string& body) {
  std::filesystem::create_directories("tests/tmp_loader");
  std::ofstream c(path);
  c << body;
}

}  // namespace

TEST(CliOptions, ParsesDataDirAndRtmSettings) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--decim-x", "10", "--decim-z", "12", "--crop-x", "50", "--crop-z", "40", "--ny", "20", "--dy", "18", "--dt", "0.001", "--nt", "100", "--f0", "10", "--pml", "8", "--receiver-stride", "4", "--output", "output/a.pgm"};
  const auto o = parse_cli(argv);

  ASSERT_EQ(o.x_file, "data/x.json");
  ASSERT_EQ(o.z_file, "data/z.json");
  ASSERT_EQ(o.values_file, "data/vel.json");
  ASSERT_EQ(o.load.decim_x, 10u);
  ASSERT_EQ(o.rtm.nt, 100u);
  ASSERT_EQ(o.output_file, "output/a.pgm");
}

TEST(CliOptions, RejectsUnknownArgument) {
  const char* argv[] = {"rtm3d_cli", "--unknown", "x"};
  EXPECT_THROW((void)parse_cli(argv), std::runtime_error);
}

TEST(CliOptions, RejectsMissingInput) {
  const char* argv[] = {"rtm3d_cli", "--nt", "100"};
  EXPECT_THROW((void)parse_cli(argv), std::runtime_error);
}

TEST(CliOptions, ParsesConfigFileAndFormat) {
  write_config_file("tests/tmp_loader/cfg.json",
                    "{\n"
                    "  \"data_dir\": \"data\",\n"
                    "  \"output_file\": \"output/out.bin\",\n"
                    "  \"output_format\": \"float32_raw\",\n"
                    "  \"nt\": 90\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg.json"};
  const auto o = parse_cli(argv);
  ASSERT_EQ(o.values_file, "data/vel.json");
  ASSERT_EQ(o.output_file, "output/out.bin");
  ASSERT_EQ(o.rtm.nt, 90u);
  ASSERT_EQ(o.output_format, rtm3d::OutputFormat::kFloat32Raw);
}

TEST(CliOptions, ExplicitInputPathOverridesDataDirDefault) {
  const char* argv[] = {"rtm3d_cli",
                        "--data-dir",
                        "data",
                        "--x-file",
                        "custom/x.json"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.x_file, "custom/x.json");
  ASSERT_EQ(o.z_file, "data/z.json");
  ASSERT_EQ(o.values_file, "data/vel.json");
}

TEST(CliOptions, ExplicitOutputOverridesConfigOutputFile) {
  write_config_file("tests/tmp_loader/cfg_output_base.json",
                    "{\n"
                    "  \"data_dir\": \"data\",\n"
                    "  \"output_file\": \"output/from_config.pgm\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli",
                        "--config",
                        "tests/tmp_loader/cfg_output_base.json",
                        "--output",
                        "output/from_cli.pgm"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.output_file, "output/from_cli.pgm");
}

TEST(CliOptions, ExplicitOutputFormatOverridesConfigOutputFormat) {
  write_config_file("tests/tmp_loader/cfg_output_format_base.json",
                    "{\n"
                    "  \"data_dir\": \"data\",\n"
                    "  \"output_format\": \"pgm8\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli",
                        "--config",
                        "tests/tmp_loader/cfg_output_format_base.json",
                        "--output-format",
                        "float32_raw"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.output_format, rtm3d::OutputFormat::kFloat32Raw);
}

TEST(CliOptions, LaterDataDirOverridesEarlierExplicitInputPath) {
  const char* argv[] = {"rtm3d_cli",
                        "--x-file",
                        "custom/x.json",
                        "--data-dir",
                        "data"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.x_file, "data/x.json");
  ASSERT_EQ(o.z_file, "data/z.json");
  ASSERT_EQ(o.values_file, "data/vel.json");
}

TEST(CliOptions, LaterConfigOverridesEarlierDataDir) {
  write_config_file("tests/tmp_loader/cfg_override_data_dir.json",
                    "{\n"
                    "  \"data_dir\": \"custom_data\",\n"
                    "  \"x_file\": \"cfg/x.json\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli",
                        "--data-dir",
                        "data",
                        "--config",
                        "tests/tmp_loader/cfg_override_data_dir.json"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.x_file, "cfg/x.json");
  ASSERT_EQ(o.z_file, "custom_data/z.json");
  ASSERT_EQ(o.values_file, "custom_data/vel.json");
}

TEST(CliOptions, LaterConfigOverridesEarlierOutputFormat) {
  write_config_file("tests/tmp_loader/cfg_override_output_format.json",
                    "{\n"
                    "  \"data_dir\": \"data\",\n"
                    "  \"output_format\": \"pgm8\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli",
                        "--data-dir",
                        "data",
                        "--output-format",
                        "float32_raw",
                        "--config",
                        "tests/tmp_loader/cfg_override_output_format.json"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.output_format, rtm3d::OutputFormat::kPgm8);
}

TEST(CliOptions, LaterCliOutputFormatOverridesEarlierConfig) {
  write_config_file("tests/tmp_loader/cfg_base_output_format.json",
                    "{\n"
                    "  \"data_dir\": \"data\",\n"
                    "  \"output_format\": \"pgm8\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli",
                        "--config",
                        "tests/tmp_loader/cfg_base_output_format.json",
                        "--output-format",
                        "float32_raw"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.output_format, rtm3d::OutputFormat::kFloat32Raw);
}

TEST(CliOptions, LaterConfigOutputOverridesEarlierCliOutput) {
  write_config_file("tests/tmp_loader/cfg_later_output.json",
                    "{\n"
                    "  \"data_dir\": \"data\",\n"
                    "  \"output_file\": \"output/from_later_config.pgm\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli",
                        "--data-dir",
                        "data",
                        "--output",
                        "output/from_earlier_cli.pgm",
                        "--config",
                        "tests/tmp_loader/cfg_later_output.json"};
  const auto o =
      rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)),
                                const_cast<char**>(argv));

  ASSERT_EQ(o.output_file, "output/from_later_config.pgm");
}

TEST(CliOptions, ParsesMinimalConfig) {
  write_config_file("tests/tmp_loader/minimal.json",
                    "{\n"
                    "  \"data_dir\": \"data\"\n"
                    "}\n");

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/minimal.json"};
  const auto o = parse_cli(argv);

  ASSERT_EQ(o.values_file, "data/vel.json");
}

TEST(CliOptions, AllRtmParametersParsedCorrectly) {
  const char* argv[] = {"rtm3d_cli",
                        "--data-dir", "data",
                        "--decim-x", "5",
                        "--decim-z", "7",
                        "--crop-x", "100",
                        "--crop-z", "80",
                        "--ny", "30",
                        "--dy", "12.5",
                        "--dt", "0.002",
                        "--nt", "200",
                        "--f0", "15.5",
                        "--pml", "12",
                        "--receiver-stride", "8",
                        "--output", "output/test.pgm"};
  const auto o = parse_cli(argv);

  EXPECT_EQ(o.load.decim_x, 5u);
  EXPECT_EQ(o.load.decim_z, 7u);
  EXPECT_EQ(o.load.crop_x, 100u);
  EXPECT_EQ(o.load.crop_z, 80u);
  EXPECT_EQ(o.rtm.ny, 30u);
  EXPECT_FLOAT_EQ(o.rtm.dy, 12.5f);
  EXPECT_FLOAT_EQ(o.rtm.dt, 0.002f);
  EXPECT_EQ(o.rtm.nt, 200u);
  EXPECT_FLOAT_EQ(o.rtm.f0, 15.5f);
  EXPECT_EQ(o.rtm.pml, 12u);
  EXPECT_EQ(o.rtm.receiver_stride, 8u);
}

TEST(CliOptions, NegativeDecimationRejected) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--decim-x", "-5"};
  EXPECT_THROW((void)parse_cli(argv), std::runtime_error);
}

TEST(CliOptions, NegativeCropRejected) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--crop-z", "-10"};
  EXPECT_THROW((void)parse_cli(argv), std::runtime_error);
}

TEST(CliOptions, ZeroNtRejected) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--nt", "0"};
  EXPECT_THROW((void)parse_cli(argv), std::runtime_error);
}

TEST(CliOptions, InvalidOutputFormatRejected) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--output-format", "invalid"};
  EXPECT_THROW((void)parse_cli(argv), std::runtime_error);
}
