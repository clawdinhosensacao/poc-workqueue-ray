# Data provenance: Marmousi benchmark

## Primary source used by this project
This repository downloads JSON arrays from:
- Repository: https://github.com/mlamoureux/Marmousi
- Files: `x.json`, `z.json`, `vel.json`
- Download script: `scripts/download_marmousi.sh`

## Provenance process
1. Download raw files directly from upstream GitHub raw URLs.
2. Save local SHA-256 checksums into `data/SHA256SUMS.txt`.
3. Use these checksums to record exact snapshots used for runs/tests.

## Licensing / availability caveat
The JSON mirror source does not clearly include a full legal license text in this project baseline.
For research/production usage, verify the original Marmousi dataset redistribution terms from official providers (historically IFP/SEG/EAGE related distributions) before downstream redistribution.

## Fallback when source is unavailable or license is unclear
Use `scripts/generate_synthetic_model.py` to create deterministic synthetic velocity models for CI and local validation. This avoids license ambiguity for automated tests.
