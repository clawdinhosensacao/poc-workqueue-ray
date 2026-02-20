#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/cli/CliOptions.hpp"

TEST(CliOptions, ParsesDataDirAndRtmSettings) {
  const char* argv[] = {"rtm3d_cli", "--data-dir", "data", "--decim-x", "10", "--decim-z", "12", "--crop-x", "50", "--crop-z", "40", "--ny", "20", "--dy", "18", "--dt", "0.001", "--nt", "100", "--f0", "10", "--pml", "8", "--receiver-stride", "4", "--output", "output/a.pgm"};
  const auto o = rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv));

  ASSERT_EQ(o.x_file, "data/x.json");
  ASSERT_EQ(o.z_file, "data/z.json");
  ASSERT_EQ(o.values_file, "data/vel.json");
  ASSERT_EQ(o.load.decim_x, 10);
  ASSERT_EQ(o.rtm.nt, 100);
  ASSERT_EQ(o.output_file, "output/a.pgm");
}

TEST(CliOptions, RejectsUnknownArgument) {
  const char* argv[] = {"rtm3d_cli", "--unknown", "x"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv)), std::runtime_error);
}

TEST(CliOptions, RejectsMissingInput) {
  const char* argv[] = {"rtm3d_cli", "--nt", "100"};
  EXPECT_THROW((void)rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv)), std::runtime_error);
}

TEST(CliOptions, ParsesConfigFileAndFormat) {
  std::filesystem::create_directories("tests/tmp_loader");
  {
    std::ofstream c("tests/tmp_loader/cfg.json");
    c << "{\n"
      << "  \"data_dir\": \"data\",\n"
      << "  \"output_file\": \"output/out.bin\",\n"
      << "  \"output_format\": \"float32_raw\",\n"
      << "  \"nt\": 90\n"
      << "}\n";
  }

  const char* argv[] = {"rtm3d_cli", "--config", "tests/tmp_loader/cfg.json"};
  const auto o = rtm3d::parse_cli_or_throw(static_cast<int>(std::size(argv)), const_cast<char**>(argv));
  ASSERT_EQ(o.values_file, "data/vel.json");
  ASSERT_EQ(o.output_file, "output/out.bin");
  ASSERT_EQ(o.rtm.nt, 90u);
  ASSERT_EQ(o.output_format, rtm3d::OutputFormat::kFloat32Raw);
}
