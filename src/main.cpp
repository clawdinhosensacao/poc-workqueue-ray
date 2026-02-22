#include <filesystem>
#include <iostream>

#include "rtm3d/cli/CliOptions.hpp"
#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/io/ImageIO.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace {

rtm3d::MigrationResult run_rtm(const rtm3d::GridModel2D& model, const rtm3d::RtmConfig& cfg, std::size_t n_shots) {
  if (n_shots <= 1) {
    return rtm3d::run_single_shot_rtm(model, cfg);
  }

  // Generate evenly spaced shot positions
  std::vector<rtm3d::ShotPosition> shots;
  shots.reserve(n_shots);
  const std::size_t sx_min = 4;
  const std::size_t sx_max = model.nx - 4;
  for (std::size_t i = 0; i < n_shots; ++i) {
    const std::size_t sx = (n_shots == 1) ? model.nx / 2 : sx_min + i * (sx_max - sx_min) / (n_shots - 1);
    shots.push_back({.sx = sx, .sz = 2});
  }
  return rtm3d::run_multi_shot_rtm(model, cfg, shots);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const auto cli = rtm3d::parse_cli_or_throw(argc, argv);

    const auto model = rtm3d::load_grid_model_from_json_arrays(cli.x_file, cli.z_file, cli.values_file, cli.load);
    const auto migration = run_rtm(model, cli.rtm, cli.n_shots);

    std::filesystem::create_directories(std::filesystem::path(cli.output_file).parent_path());
    if (cli.output_format == rtm3d::OutputFormat::kFloat32Raw) {
      rtm3d::write_float32_raw(cli.output_file, migration.inline_xz, migration.nx, migration.nz);
    } else {
      rtm3d::write_pgm(cli.output_file, migration.inline_xz, migration.nx, migration.nz);
    }

    std::cout << "RTM finished\n"
              << "model nx=" << model.nx << " nz=" << model.nz << " dx=" << model.dx << " dz=" << model.dz << "\n"
              << "shots=" << cli.n_shots << "\n"
              << "output=" << cli.output_file << "\n";
    return 0;
  } catch (const std::exception& e) {
    const std::string m = e.what();
    if (m.rfind("Usage:", 0) == 0) {
      std::cout << m;
      return 0;
    }
    std::cerr << "error: " << e.what() << "\n\n" << rtm3d::cli_help();
    return 2;
  }
}
