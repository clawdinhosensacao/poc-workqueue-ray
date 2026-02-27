/**
 * @file CliOptions.hpp
 * @brief Command-line interface configuration for rtm3d-cli.
 *
 * Provides argument parsing and configuration structures for the
 * RTM migration command-line tool.
 */

#pragma once

#include <string>

#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d {

/// @brief Project version string (major.minor.patch)
inline constexpr const char* kVersion = "0.2.1";

/// @brief Output format for migrated images
enum class OutputFormat {
  kPgm8,        ///< 8-bit PGM grayscale image
  kFloat32Raw   ///< Raw float32 binary with JSON header
};

/**
 * @brief Parsed command-line options for RTM migration.
 *
 * Contains all configuration needed to run an RTM migration,
 * including input/output paths, model loading options, and RTM parameters.
 */
struct CliOptions {
  std::string x_file;                          ///< X coordinates JSON file
  std::string z_file;                          ///< Z coordinates JSON file
  std::string values_file;                     ///< Velocity values JSON file
  std::string output_file = "output/migrated_inline.pgm";  ///< Output path
  OutputFormat output_format = OutputFormat::kPgm8;        ///< Output format
  GridLoadOptions load;                        ///< Model loading options
  RtmConfig rtm;                               ///< RTM configuration
  std::size_t n_shots = 1;  ///< Number of shots (1=single-shot, >1=multi-shot evenly spaced)
};

/**
 * @brief Parse command-line arguments.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Parsed options
 * @throws std::runtime_error on invalid arguments (shows help)
 */
CliOptions parse_cli_or_throw(int argc, char** argv);

/// @brief Get help text for command-line usage
std::string cli_help();

/// @brief Get version string
std::string cli_version();

}  // namespace rtm3d
