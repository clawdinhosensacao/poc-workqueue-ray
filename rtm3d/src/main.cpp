#include <filesystem>
#include <iostream>

#include "rtm3d/cli/CliOptions.hpp"
#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/io/ImageIO.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

int main(int argc, char** argv) {
  try {
    const auto cli = rtm3d::parse_cli_or_throw(argc, argv);

    const auto model = rtm3d::load_grid_model_from_json_arrays(cli.x_file, cli.z_file, cli.values_file, cli.load);
    const auto migration = rtm3d::run_single_shot_rtm(model, cli.rtm);

    std::filesystem::create_directories(std::filesystem::path(cli.output_file).parent_path());
    rtm3d::write_pgm(cli.output_file, migration.inline_xz, migration.nx, migration.nz);

    std::cout << "RTM finished\n"
              << "model nx=" << model.nx << " nz=" << model.nz << " dx=" << model.dx << " dz=" << model.dz << "\n"
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
