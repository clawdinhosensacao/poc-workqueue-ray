#include "rtm3d/cli/CliOptions.hpp"

#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace rtm3d {
namespace {

bool starts_with(const std::string& token, const std::string& prefix) {
  return token.rfind(prefix, 0) == 0;
}

bool is_flag(const std::string& token) { return starts_with(token, "--"); }

bool is_help_flag(const std::string& token) {
  return token == "--help" || token == "-h";
}

bool is_version_flag(const std::string& token) {
  return token == "--version" || token == "-V";
}

bool is_input_source_option(const std::string& token) {
  return token == "--config" || token == "--data-dir";
}

bool is_input_path_option(const std::string& token) {
  return token == "--x-file" || token == "--z-file" ||
         token == "--values-file";
}

bool is_output_option(const std::string& token) {
  return token == "--output" || token == "--output-format";
}

std::string require_value(int argc, char** argv, int& i) {
  if (i + 1 >= argc) throw std::runtime_error("missing value for " + std::string(argv[i]));
  return argv[++i];
}

std::string slurp_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("cannot open config file: " + path);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string json_find_string(const std::string& s, const std::string& key) {
  const std::regex rx("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
  std::smatch m;
  if (std::regex_search(s, m, rx)) return m[1].str();
  return "";
}

std::string json_find_number_token(const std::string& s, const std::string& key) {
  const std::regex rx("\\\"" + key + "\\\"\\s*:\\s*([-+0-9eE\\.]+)");
  std::smatch m;
  if (std::regex_search(s, m, rx)) return m[1].str();
  return "";
}

void apply_data_dir(CliOptions& o, const std::string& dir) {
  o.x_file = dir + "/x.json";
  o.z_file = dir + "/z.json";
  o.values_file = dir + "/vel.json";
}

OutputFormat parse_output_format_or_throw(const std::string& token, const std::string& source) {
  if (token == "pgm8") return OutputFormat::kPgm8;
  if (token == "float32_raw") return OutputFormat::kFloat32Raw;
  throw std::runtime_error("invalid output format in " + source + ": " + token);
}

OutputFormat parse_cli_output_format_or_throw(const std::string& token) {
  return parse_output_format_or_throw(token, "--output-format");
}

template <typename T>
T parse_num(const std::string& s, const std::string& name);

void apply_json_config(CliOptions& o, const std::string& path);

std::runtime_error invalid_value_error(const std::string& name,
                                       const std::string& value) {
  return std::runtime_error("invalid value for " + name + ": " + value);
}

template <>
std::size_t parse_num<std::size_t>(const std::string& s, const std::string& name) {
  if (s.empty() || s.front() == '-') {
    throw invalid_value_error(name, s);
  }

  try {
    std::size_t p = 0;
    auto v = std::stoull(s, &p);
    if (p != s.size()) {
      throw invalid_value_error(name, s);
    }
    return static_cast<std::size_t>(v);
  } catch (const std::exception&) {
    throw invalid_value_error(name, s);
  }
}

template <>
float parse_num<float>(const std::string& s, const std::string& name) {
  try {
    std::size_t p = 0;
    auto v = std::stof(s, &p);
    if (p != s.size()) {
      throw invalid_value_error(name, s);
    }
    return v;
  } catch (const std::exception&) {
    throw invalid_value_error(name, s);
  }
}

void apply_input_paths_from_json(const std::string& s, CliOptions& o) {
  if (const auto v = json_find_string(s, "x_file"); !v.empty()) o.x_file = v;
  if (const auto v = json_find_string(s, "z_file"); !v.empty()) o.z_file = v;
  if (const auto v = json_find_string(s, "values_file"); !v.empty()) o.values_file = v;
}

void apply_cli_input_source_option(const std::string& arg,
                                    const std::string& value,
                                    CliOptions& o) {
  if (arg == "--config") {
    apply_json_config(o, value);
  } else if (arg == "--data-dir") {
    apply_data_dir(o, value);
  }
}

void apply_cli_input_path_option(const std::string& arg,
                                 const std::string& value,
                                 CliOptions& o) {
  if (arg == "--x-file") {
    o.x_file = value;
  } else if (arg == "--z-file") {
    o.z_file = value;
  } else if (arg == "--values-file") {
    o.values_file = value;
  }
}

void apply_cli_output_option(const std::string& arg,
                             const std::string& value,
                             CliOptions& o) {
  if (arg == "--output") {
    o.output_file = value;
  } else if (arg == "--output-format") {
    o.output_format = parse_cli_output_format_or_throw(value);
  }
}

void apply_output_path_aliases_from_json(const std::string& s, CliOptions& o) {
  if (const auto v = json_find_string(s, "output_file"); !v.empty()) o.output_file = v;
  if (const auto v = json_find_string(s, "output"); !v.empty()) o.output_file = v;  // alias
}

void apply_json_config(CliOptions& o, const std::string& path) {
  const auto s = slurp_file(path);

  const auto set_numeric_if_present =
      [&](const std::string& key, auto& out) {
        if (const auto v = json_find_number_token(s, key); !v.empty()) {
          out = parse_num<std::decay_t<decltype(out)>>(v, key);
        }
      };

  if (const auto dir = json_find_string(s, "data_dir"); !dir.empty()) {
    apply_data_dir(o, dir);
  }
  apply_input_paths_from_json(s, o);
  apply_output_path_aliases_from_json(s, o);

  if (const auto v = json_find_string(s, "output_format"); !v.empty()) {
    o.output_format = parse_output_format_or_throw(v, "config");
  }

  set_numeric_if_present("decim_x", o.load.decim_x);
  set_numeric_if_present("decim_z", o.load.decim_z);
  set_numeric_if_present("crop_x", o.load.crop_x);
  set_numeric_if_present("crop_z", o.load.crop_z);

  set_numeric_if_present("ny", o.rtm.ny);
  set_numeric_if_present("dy", o.rtm.dy);
  set_numeric_if_present("dt", o.rtm.dt);
  set_numeric_if_present("nt", o.rtm.nt);
  set_numeric_if_present("f0", o.rtm.f0);
  set_numeric_if_present("pml", o.rtm.pml);
  set_numeric_if_present("receiver_stride", o.rtm.receiver_stride);
  set_numeric_if_present("n_shots", o.n_shots);
}

void validate_input_files(const CliOptions& o) {
  if (o.x_file.empty() || o.z_file.empty() || o.values_file.empty()) {
    throw std::runtime_error("x/z/values input files are required (or --data-dir / --config)");
  }
}

void require_gt_zero(float value, const std::string& name) {
  if (value <= 0) {
    throw std::runtime_error(name + " must be > 0");
  }
}

void require_gt_zero(std::size_t value, const std::string& name) {
  if (value == 0) {
    throw std::runtime_error(name + " must be > 0");
  }
}

void validate_load_options(const CliOptions& o) {
  if (o.load.decim_x == 0 || o.load.decim_z == 0) {
    throw std::runtime_error("decimation must be >= 1");
  }
}

void validate_rtm_options(const CliOptions& o) {
  if (o.rtm.ny < 4 || o.rtm.nt < 2) throw std::runtime_error("ny>=4 and nt>=2 required");
  require_gt_zero(o.rtm.dy, "dy");
  require_gt_zero(o.rtm.dt, "dt");
  require_gt_zero(o.rtm.f0, "f0");
  require_gt_zero(o.rtm.pml, "pml");
  require_gt_zero(o.rtm.receiver_stride, "receiver-stride");
}

void validate(const CliOptions& o) {
  validate_input_files(o);
  validate_load_options(o);
  validate_rtm_options(o);
}

}  // namespace

std::string cli_help() {
  return "Usage: rtm3d_cli [options]\n"
         "Input model:\n"
         "  --config <file.json>          JSON config file (recommended)\n"
         "  --data-dir <dir>              Directory containing x.json z.json vel.json\n"
         "  --x-file <path>               X axis JSON array\n"
         "  --z-file <path>               Z axis JSON array\n"
         "  --values-file <path>          2D values JSON array\n"
         "Load options:\n"
         "  --decim-x <n> --decim-z <n>   Decimation factors (>=1)\n"
         "  --crop-x <n> --crop-z <n>     Crop size (0 means full)\n"
         "RTM options:\n"
         "  --ny <n> --dy <m> --dt <s> --nt <n> --f0 <Hz> --pml <n> --receiver-stride <n>\n"
         "  --n-shots <n>                 Number of shots (default=1, evenly spaced sources)\n"
         "Output:\n"
         "  --output <path>               Output file path\n"
         "  --output-format <pgm8|float32_raw>\n"
         "Other:\n"
         "  --help                        Show this message\n"
         "  --version                     Show version\n";
}

std::string cli_version() { return std::string("rtm3d-cli ") + kVersion + "\n"; }

CliOptions parse_cli_or_throw(int argc, char** argv) {
  CliOptions o;

  const auto parse_string_option = [&](int& idx) {
    return require_value(argc, argv, idx);
  };
  const auto parse_size_option = [&](int& idx, const std::string& name) {
    return parse_num<std::size_t>(parse_string_option(idx), name);
  };
  const auto parse_float_option = [&](int& idx, const std::string& name) {
    return parse_num<float>(parse_string_option(idx), name);
  };

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (is_help_flag(arg)) throw std::runtime_error(cli_help());
    if (is_version_flag(arg)) throw std::runtime_error(cli_version());

    if (is_input_source_option(arg)) {
      apply_cli_input_source_option(arg, parse_string_option(i), o);
    } else if (is_input_path_option(arg)) {
      apply_cli_input_path_option(arg, parse_string_option(i), o);
    } else if (is_output_option(arg)) {
      apply_cli_output_option(arg, parse_string_option(i), o);
    } else if (arg == "--decim-x") {
      o.load.decim_x = parse_size_option(i, "--decim-x");
    } else if (arg == "--decim-z") {
      o.load.decim_z = parse_size_option(i, "--decim-z");
    } else if (arg == "--crop-x") {
      o.load.crop_x = parse_size_option(i, "--crop-x");
    } else if (arg == "--crop-z") {
      o.load.crop_z = parse_size_option(i, "--crop-z");
    } else if (arg == "--ny") {
      o.rtm.ny = parse_size_option(i, "--ny");
    } else if (arg == "--dy") {
      o.rtm.dy = parse_float_option(i, "--dy");
    } else if (arg == "--dt") {
      o.rtm.dt = parse_float_option(i, "--dt");
    } else if (arg == "--nt") {
      o.rtm.nt = parse_size_option(i, "--nt");
    } else if (arg == "--f0") {
      o.rtm.f0 = parse_float_option(i, "--f0");
    } else if (arg == "--pml") {
      o.rtm.pml = parse_size_option(i, "--pml");
    } else if (arg == "--receiver-stride") {
      o.rtm.receiver_stride = parse_size_option(i, "--receiver-stride");
    } else if (arg == "--n-shots") {
      o.n_shots = parse_size_option(i, "--n-shots");
    } else if (is_flag(arg)) {
      throw std::runtime_error("unknown option: " + arg);
    } else {
      throw std::runtime_error("unexpected positional argument: " + arg);
    }
  }

  validate(o);
  return o;
}

}  // namespace rtm3d
