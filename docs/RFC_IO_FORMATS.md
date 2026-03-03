# RFC: I/O Backends for RTM Models (HDF5, ADIOS2, Parquet, DuckDB, Zarr, MDIO)

Status: Draft  
Author: rtm3d-cli maintenance stream  
Date: 2026-03-03

## 1) Context

`rtm3d-cli` currently gets best throughput from raw float32 binary and mmap-based reads. That is ideal for local single-node hot paths, but users asked for an objective comparison with ecosystem formats (HDF5, ADIOS2, Parquet, DuckDB, Zarr, MDIO) and clear guidance about when each is appropriate.

## 2) Goals

- Compare candidate formats with the **same benchmark harness**.
- Keep current fast path (raw binary/mmap) first-class.
- Add pragmatic guidance: which format for which workload.
- Avoid lock-in: treat advanced formats as optional integrations.

Non-goals:

- Replace all existing binary I/O.
- Promise universal dependency availability on every machine.

## 3) Implemented in this RFC cycle

### 3.1 Benchmark harness

New script: `scripts/io_format_benchmark.py`

- Benchmarks write/read for:
  - `json`
  - `binary_f32`
  - `npy`
  - `parquet` *(optional pyarrow)*
  - `duckdb` *(optional duckdb)*
  - `hdf5` *(optional h5py)*
  - `zarr` *(optional zarr)*
  - `adios2` *(optional adios2)*
  - `mdio` *(best-effort placeholder, API version dependent)*
- Produces markdown report (default): `artifacts/io_format_benchmark.md`
- Handles unavailable deps gracefully (marks `n/a`, does not crash whole run).

### 3.2 Tests

New test file: `scripts/test_io_format_benchmark.py`

- Verifies markdown report formatting.
- Verifies core always-available formats (`json`, `binary_f32`, `npy`) execute and emit non-zero output size.

## 4) How to run

```bash
python3 scripts/io_format_benchmark.py --nx 400 --nz 300 --iterations 3
python3 -m unittest scripts/test_io_format_benchmark.py
```

## 5) Format comparison (practical)

### Raw binary float32 (+ mmap)

Best for:
- Maximum sequential throughput
- Low overhead pipelines
- Tight coupling with C++ compute kernels

Tradeoffs:
- Weak metadata story unless paired with sidecar/header
- Less self-describing than container formats

### HDF5

Best for:
- Scientific datasets with rich metadata
- Multi-dataset packaging in one file
- Mature ecosystem in HPC/science

Tradeoffs:
- Single-file write contention patterns can be tricky
- Dependency/runtime complexity vs raw binary

### ADIOS2

Best for:
- HPC/distributed workflows
- High-performance staging/checkpoint pipelines

Tradeoffs:
- Operational/dependency complexity
- Usually overkill for simple local CLI workflows

### Zarr

Best for:
- Chunked cloud/object-store-friendly arrays
- Python-first data ecosystem

Tradeoffs:
- Small-file/chunk management overhead locally
- Requires chunking strategy tuning

### Parquet

Best for:
- Analytics/tabular interoperability
- SQL/data-lake ecosystems

Tradeoffs:
- Not a native multidimensional scientific array format
- Usually requires flatten/reshape conventions

### DuckDB

Best for:
- Local analytical queries and ad-hoc filtering
- SQL-based introspection workflows

Tradeoffs:
- Not intended as the fastest dense-array transport layer
- Conversion overhead for pure numeric stencil workloads

### MDIO

Best for:
- Domain-specific seismic metadata+data workflows (where adopted)

Tradeoffs:
- Ecosystem/version variability
- Integration details depend on concrete MDIO API version

## 6) Decision guidance

Use **raw binary/mmap** when:
- You need maximum RTM ingest speed and simple deployment.

Use **HDF5 or Zarr** when:
- You need self-describing structured scientific datasets, metadata, and broader interchange.

Use **ADIOS2** when:
- You run distributed/HPC workflows where ADIOS2 is already standard.

Use **Parquet/DuckDB** when:
- Primary need is analytics/reporting/querying rather than pure stencil-compute I/O throughput.

Use **MDIO** when:
- Team standardizes on seismic-domain MDIO tooling and compatibility matters more than minimal dependencies.

## 7) Proposed staged adoption

1. Keep binary/mmap as default hot path.
2. Keep benchmark script in CI optional job (dependency-aware).
3. Add one container backend first (recommended: HDF5 or Zarr) behind explicit feature flag.
4. Re-evaluate ADIOS2/MDIO integration based on user demand + deployment context.

## 8) Risks and mitigations

- Risk: dependency sprawl
  - Mitigation: optional adapters + graceful skip.
- Risk: confusion over “fastest” claims across machines
  - Mitigation: run local benchmark report per environment.
- Risk: accidental regression of hot path
  - Mitigation: keep binary/mmap tests and benchmark baseline as reference.

## 9) Acceptance criteria for this RFC cycle

- [x] Multi-format benchmark harness added.
- [x] Optional dependency behavior explicit and non-fatal.
- [x] Automated tests for benchmark core paths.
- [x] Written guidance on when each format applies.
