# rtm3d-cli (production baseline)

C++20 command-line baseline for **3D acoustic RTM** (isotropic, CPU) with architecture prepared for future anisotropy (TTI) and GPU backends.

## Highlights
- Generic model I/O abstraction (`ArrayModelLoader` + `GridModelLoader`) not tied to Marmousi naming.
- Strict CLI argument parsing and validation (no hardcoded runtime values).
- GoogleTest test suite with unit + integration-style coverage.
- Data provenance + reproducible Marmousi download script.

## Repository layout
- `include/rtm3d/model` – model structs (`GridModel2D`)
- `include/rtm3d/io` – array parsing, grid loading, image writing
- `include/rtm3d/rtm` – RTM engine interfaces
- `include/rtm3d/cli` – CLI schema/parsing
- `src/io`, `src/rtm`, `src/cli` – implementations
- `tests` – GoogleTest suite
- `scripts` – data acquisition + helper scripts
- `docs` – architecture and provenance notes

## Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRTM3D_BUILD_TESTS=ON
cmake --build build -j
```

## Tests
```bash
ctest --test-dir build --output-on-failure
```

## Download real Marmousi data
```bash
./scripts/download_marmousi.sh data
```

See `docs/DATA_PROVENANCE.md` for source and licensing caveats.

## Run RTM
```bash
./build/rtm3d_cli \
  --data-dir data \
  --decim-x 20 --decim-z 20 \
  --crop-x 80 --crop-z 50 \
  --ny 24 --dy 20 --dt 0.0015 --nt 140 --f0 12 --pml 8 --receiver-stride 5 \
  --output output/migrated_inline.pgm
```

## Confidence checks
- End-to-end run should produce `output/migrated_inline.pgm` and non-zero migrated image energy.
- `ctest` verifies parser, CLI validation, RTM execution, and output file writing.

## About PGM output
PGM (Portable GrayMap, `P5` binary) is a minimal grayscale raster format with tiny implementation overhead and broad compatibility.

**Pros:** very simple, deterministic, good for debug snapshots.
**Cons:** no metadata, no compression by default, 8-bit output in this implementation.

### Better output options for production
- **TIFF / GeoTIFF**: richer metadata, higher bit depth, industry tooling.
- **HDF5 / NetCDF / Zarr**: multidimensional arrays, chunking/compression, scalable workflows.
- **SEG-Y (for seismic traces/volumes)**: domain standard interchange format.
- **PNG**: convenient preview format, but not ideal for scientific dynamic range.

## Future-ready notes
Current solver is isotropic CPU-only, but interfaces are split by concern (`io`, `rtm`, `cli`) so adding:
- `RtmEngineTTI` (anisotropy)
- GPU backend (`CudaRtmEngine` / `HipRtmEngine`)
can be done without changing CLI/model IO contracts.
