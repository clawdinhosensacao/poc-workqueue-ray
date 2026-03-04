# Architecture

## Modules
- **model/**: core data shapes (`GridModel2D`).
- **io/**:
  - `ArrayModelLoader`: generic JSON array ingestion.
  - `GridModelLoader`: maps axes + 2D arrays into uniform grid models with decimation/cropping.
  - `ImageIO`: image output helpers.
- **rtm/**: finite-difference isotropic CPU RTM.
- **cli/**: argument parsing and validation boundary.

## Why this split helps future TTI/GPU
- RTM internals can be replaced while preserving external IO + CLI contracts.
- Loader remains format-focused rather than benchmark-focused (Marmousi is now an input instance, not a code concept).
- Tests isolate concerns (CLI/IO/solver) to reduce regressions during backend upgrades.
