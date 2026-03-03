# I/O Performance Guide

This document describes the I/O performance characteristics of `rtm3d-cli` and provides guidance for optimal data formats.

## Performance Summary

| Format | Read Speed | Write Speed | Size Overhead | Use Case |
|--------|-----------|-------------|---------------|----------|
| JSON array | ~40 MB/s | ~20 MB/s | 5x | Human-readable, small models |
| Binary float32 | ~1200 MB/s | ~664 MB/s | 1x | Production, large models |
| Memory-mapped | ~1200 MB/s* | N/A | 1x | Zero-copy, streaming |

\* Memory-mapped read speed depends on access pattern; sequential access is fastest.

## Benchmark Results

### JSON vs Binary Comparison

Tested with Python's `json` module and NumPy binary I/O:

| Array Size | JSON Time | Binary Time | Speedup | Size Ratio |
|------------|-----------|-------------|---------|------------|
| 100x100 | 4.2 ms | 0.1 ms | 34x | 5.1x |
| 200x200 | 18.2 ms | 0.5 ms | 40x | 5.1x |
| 400x300 | 50.0 ms | 1.1 ms | 45x | 5.1x |

### Binary I/O Throughput

| Data Size | Write Speed | Read Speed |
|-----------|-------------|------------|
| 0.04 MB | 52 MB/s | 283 MB/s |
| 0.38 MB | 345 MB/s | 2881 MB/s |
| 3.81 MB | 527 MB/s | 1587 MB/s |
| 38.15 MB | 664 MB/s | 1201 MB/s |

## Recommendations

### For Small Models (< 1 MB)
- JSON format is acceptable for human readability
- Use `--x-file`, `--z-file`, `--values-file` CLI options

### For Large Models (> 1 MB)
- **Strongly recommended**: Use binary float32 format
- Use `load_binary_velocity()` API or `SeismicModel::from_file()`
- Memory-mapped I/O for zero-copy access

### For Very Large Models (> 100 MB)
- Use `MappedVelocityModel` for memory-mapped I/O
- OS handles paging automatically
- No need to fit entire model in RAM

## File Formats

### Binary Float32 (No Header)
```
[float32][float32][float32]... (row-major: [nz][nx][ny])
```
Dimensions must be known separately (via CLI or config).

### Binary Float32 with Header
```
[BinaryVelocityHeader (72 bytes)]
[float32][float32][float32]... (row-major)
```
Header contains: magic number, version, nx, nz, ny, dx, dz, dy

### JSON Arrays
```json
// 1D axis (x.json, z.json)
[0.0, 10.0, 20.0, ...]

// 2D values (vel.json)
[[v00, v01, ...], [v10, v11, ...], ...]
```

## API Usage

### Simple Binary Load
```cpp
#include "rtm3d/io/BinaryModelLoader.hpp"

rtm3d::GridSpec grid{.nx = 100, .nz = 80, .ny = 1, .dx = 10, .dz = 10};
auto model = rtm3d::load_binary_velocity("velocity.bin", grid);
```

### Memory-Mapped Load (Zero-Copy)
```cpp
#include "rtm3d/io/BinaryModelLoader.hpp"

auto mapped = rtm3d::mmap_binary_velocity("velocity.bin", grid);
float value = mapped->at(i, k, j);  // Access by index
const float* data = mapped->data(); // Raw pointer
```

### Write Binary
```cpp
rtm3d::write_binary_velocity("output.bin", model, true); // with header
rtm3d::write_binary_velocity("output.bin", model, false); // without header
```

## Running the Benchmark

```bash
python3 scripts/benchmark_io.py
```

This will output detailed timing and throughput metrics for various I/O operations.

## Multi-Format Benchmark (RFC companion)

To compare additional backends (HDF5, ADIOS2, Parquet, DuckDB, Zarr, MDIO) use:

```bash
python3 scripts/io_format_benchmark.py --nx 400 --nz 300 --iterations 3
```

Output report: `artifacts/io_format_benchmark.md`.

If optional dependencies are missing, the script marks those formats as `n/a` instead of failing.

See also: `docs/RFC_IO_FORMATS.md` for recommendations on when each format applies.
