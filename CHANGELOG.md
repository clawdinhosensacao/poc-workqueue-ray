# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- Full I/O format benchmark (14 formats): binary_f32, npy, hdf5, netcdf, parquet, duckdb, zarr, tiledb, segy, asdf, adios2, tensorstore, mdio, json
- Benchmark ranking helpers for read/write/balanced throughput with deterministic tie-breaking
- Markdown report with availability stats, top-K rankings, and fastest format summaries
- 30 Python unit tests for I/O benchmark harness (io_format_benchmark.py)
- Dependencies: h5py, zarr, pyarrow, duckdb, netCDF4, tiledb, segyio, asdf, tensorstore, adios2

### Changed
- Test suite expanded to 155 tests (25 test suites)
- I/O benchmark supports graceful degradation when optional dependencies missing

### Added
- `scripts/devito_canonical_parity.py`: canonical Devito RTM parity pipeline using Devito operators for forward modeling, receiver recording, reverse-time propagation, and cross-correlation imaging.
- Robust parity CLI options for grid/time/source/PML controls and JSON report output.
- Threshold gating options for parity runs (`--fail-on-threshold`, `--min-ncc`, `--min-ssim`, `--max-nrmse`).
- Configurable SSIM window via `--ssim-window`.
- Canonical parity report metadata fields: `schema_version` and `generated_at_utc`.
- `make parity-smoke` target to validate canonical parity script wiring and argument guards.
- `make check` unified verification target (unit tests + parity smoke + static analysis).

### Changed
- CI now uses `make check` as the unified validation stage before E2E.
- README updated with canonical parity workflow, thresholds, exit codes, SSIM tuning, and `make check`/`parity-smoke` guidance.
- Canonical parity report now includes full run configuration for reproducibility.
- Canonical parity argument validation hardened (grid/time/velocity/geometry/threshold/space-order/CLI path checks).
- `parity-smoke` expanded to cover invalid threshold, geometry bounds, velocity range, PML bounds, space-order, and missing CLI binary cases.
- CI setup docs aligned with current `make check` + E2E workflow.
- Preset consistency test now validates Y-slice replication across all built-in presets.

### Deprecated
- `scripts/devito_comparison.py`
- `scripts/compare_devito_rtm3d.py`
- `scripts/devito_validation.py`

## [0.2.1] - 2026-02-25

### Changed
- Applied cppcheck style fixes: replaced raw loops with STL algorithms (`std::any_of`, `std::accumulate`)
- Made `SeismicModel` single-argument constructor explicit
- Removed redundant `n_shots==1` check in multi-shot loop (always false after early return)
- Updated Makefile to detect linuxbrew-installed static analysis tools

### Quality
- Static analysis now clean: 0 warnings from cppcheck and clang-tidy
- All 62 tests passing

## [0.2.0] - 2026-02-24

### Added
- GitHub Actions CI workflow for automated testing on push/PR
- Devito cross-validation scripts (`scripts/devito_validation.py`, `scripts/cross_validate_devito.py`, `scripts/devito_comparison.py`)
- Similarity metrics (SSIM, NCC, NRMSE) for Devito vs rtm3d-cli comparison
- SeismicModel presets: `CircleLens`, `SaltDome` (diapir geometry), `Fault` (normal fault with layer offset)
- Crossline YZ and depth XY slice extraction
- Sharpness metric (Laplacian variance) in E2E pipeline
- Self-testing E2E pipeline with 6 validation checks
- `make coverage` target for gcov-based code coverage

### Changed
- PML boundary absorption coefficient increased (0.03 → 0.75) for effective absorption
- RTM architecture refactored into 10 modular components

### Fixed
- Boundary damping now properly below 0.5 threshold at edges

### Documentation
- Doxygen API documentation added to all public headers
- README updated with test count and Devito validation section
- CHANGELOG.md created following Keep a Changelog format

## [0.1.0] - 2026-02-20

### Added
- Initial 3D acoustic RTM implementation
- Single-shot and multi-shot migration
- PML absorbing boundaries
- Cross-correlation imaging condition
- CLI with JSON config support
- Synthetic benchmark pipeline
- 60 unit tests (23 test suites)
