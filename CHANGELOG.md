# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

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
