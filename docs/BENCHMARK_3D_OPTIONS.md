# 3D benchmark options (velocity + seismic input)

Goal: move from 2D Marmousi-derived smoke test to a true 3D benchmark with both velocity model and seismic input data.

## Candidate A — SEAM Phase I (preferred technical benchmark)
- Widely used 3D industry benchmark for imaging/FWI studies.
- Includes high-quality 3D earth model and synthetic seismic products in common workflows.
- Caveat: access/licensing is controlled; may require registration/approval.

## Candidate B — Open 3D seismic surveys + companion models
- Some public surveys provide 3D seismic cubes (often SEG-Y) but not always a full 3D velocity model.
- In that case, velocity must come from provided migration velocities or from synthetic setup.

## Practical integration plan in this repo
1. Add `scripts/download_benchmark_3d.sh` for the selected source.
2. Add loader interfaces for seismic shot gathers (SEG-Y or binary float32).
3. Keep model loader generic (`GridModelLoader`) and plug source-specific adapters under `src/io/`.
4. Keep provenance checksums in `data/SHA256SUMS.txt` + source notes.

## Current status
- Current repo already supports generic model loading and RTM runtime configuration via JSON config file.
- Immediate next step is selecting a legally redistributable 3D benchmark source with both velocity and seismic datasets.
