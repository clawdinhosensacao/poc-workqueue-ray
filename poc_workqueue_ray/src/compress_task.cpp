#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <zlib.h>

struct Args {
    uint64_t seed = 1234;
    size_t n = 1'000'000;
    int level = 3;
    std::string out;
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << name << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (k == "--seed") a.seed = std::stoull(next("--seed"));
        else if (k == "--n") a.n = static_cast<size_t>(std::stoull(next("--n")));
        else if (k == "--level") a.level = std::stoi(next("--level"));
        else if (k == "--out") a.out = next("--out");
        else {
            std::cerr << "Unknown arg: " << k << "\n";
            std::exit(2);
        }
    }
    return a;
}

int main(int argc, char** argv) {
    Args args = parse_args(argc, argv);

    auto t0 = std::chrono::steady_clock::now();

    std::vector<float> input(args.n);
    std::mt19937_64 rng(args.seed);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    for (auto& v : input) v = dist(rng);

    const Bytef* src = reinterpret_cast<const Bytef*>(input.data());
    uLong src_size = static_cast<uLong>(input.size() * sizeof(float));
    uLong dest_bound = compressBound(src_size);
    std::vector<Bytef> compressed(dest_bound);

    auto c0 = std::chrono::steady_clock::now();
    int rc = compress2(compressed.data(), &dest_bound, src, src_size, args.level);
    auto c1 = std::chrono::steady_clock::now();

    if (rc != Z_OK) {
        std::cerr << "compress2 failed: " << rc << "\n";
        return 1;
    }

    if (!args.out.empty()) {
        std::ofstream ofs(args.out, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(compressed.data()), static_cast<std::streamsize>(dest_bound));
    }

    auto t1 = std::chrono::steady_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double comp_ms = std::chrono::duration<double, std::milli>(c1 - c0).count();

    double ratio = static_cast<double>(src_size) / static_cast<double>(dest_bound);
    std::cout << "seed=" << args.seed
              << " n=" << args.n
              << " input_bytes=" << src_size
              << " compressed_bytes=" << dest_bound
              << " ratio=" << ratio
              << " comp_ms=" << comp_ms
              << " total_ms=" << total_ms
              << "\n";
    return 0;
}
