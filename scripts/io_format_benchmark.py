#!/usr/bin/env python3
"""Benchmark candidate I/O formats for RTM velocity/model arrays.

Goals:
- Provide apples-to-apples write/read timing across multiple storage formats.
- Work even when optional dependencies are not installed.
- Emit a Markdown summary report for RFC/design discussions.

Formats:
- json (builtin)
- binary_f32 (numpy tofile/fromfile)
- npy (numpy native format)
- parquet (optional: pyarrow)
- duckdb (optional: duckdb)
- hdf5 (optional: h5py)
- zarr (optional: zarr)
- adios2 (optional: adios2)
- mdio (optional: mdio)
"""

from __future__ import annotations

import argparse
import json
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional

import numpy as np


@dataclass
class BenchResult:
    name: str
    available: bool
    file_size_mb: float = 0.0
    write_s: float = 0.0
    read_s: float = 0.0
    write_mbps: float = 0.0
    read_mbps: float = 0.0
    error: str = ""


Writer = Callable[[np.ndarray, Path], None]
Reader = Callable[[Path, tuple[int, ...]], np.ndarray]
JobSpec = tuple[str, Path, Optional[tuple[Writer, Reader]]]
_TOP_K = 3
_TABLE_HEADER_LINES = [
    "| Format | Status | Size (MB) | Write (ms) | Read (ms) | Write MB/s | Read MB/s | Notes |",
    "|---|---:|---:|---:|---:|---:|---:|---|",
]


def _mb(path: Path) -> float:
    return path.stat().st_size / (1024 * 1024)


def _throughput(size_mb: float, seconds: float) -> float:
    if seconds <= 0:
        return 0.0
    return size_mb / seconds


def _rank_available(results: list[BenchResult], key: Callable[[BenchResult], float], top_n: int = _TOP_K) -> list[BenchResult]:
    available = _available_rows(results)
    available.sort(key=lambda r: (-key(r), r.name))
    return available[:top_n]


def _read_mbps(result: BenchResult) -> float:
    return result.read_mbps


def _write_mbps(result: BenchResult) -> float:
    return result.write_mbps


def _balanced_mbps(result: BenchResult) -> float:
    """Return a symmetric read/write score using harmonic mean throughput."""
    if result.read_mbps <= 0.0 or result.write_mbps <= 0.0:
        return 0.0
    return 2.0 / ((1.0 / result.read_mbps) + (1.0 / result.write_mbps))


def _available_rows(results: list[BenchResult]) -> list[BenchResult]:
    return [r for r in results if r.available]


def _availability_stats(results: list[BenchResult]) -> tuple[int, int, float]:
    total = len(results)
    available_count = len(_available_rows(results))
    unavailable_count = total - available_count
    available_pct = (100.0 * available_count / total) if total else 0.0
    return available_count, unavailable_count, available_pct


def _shape_label(nx: int, nz: int) -> str:
    return f"{nz} x {nx}"


def _format_fastest_summary(
    label: str,
    result: Optional[BenchResult],
    score: Callable[[BenchResult], float],
    unit_suffix: str = "MB/s",
) -> str:
    if result is None:
        return f"- {label}: `n/a`"
    return f"- {label}: `{result.name}` ({score(result):.1f} {unit_suffix})"


def _report_metadata_lines(
    nx: int,
    nz: int,
    iterations: int,
    seed: int,
    available_count: int,
    unavailable_count: int,
    available_pct: float,
    fastest_read: Optional[BenchResult],
    fastest_write: Optional[BenchResult],
    best_balanced: Optional[BenchResult],
) -> list[str]:
    return [
        f"- Shape: `{_shape_label(nx, nz)}` float32",
        f"- Iterations: `{iterations}`",
        f"- Seed: `{seed}`",
        f"- Formats available: `{available_count}`",
        f"- Formats unavailable: `{unavailable_count}`",
        f"- Availability ratio: `{available_pct:.1f}%`",
        _format_fastest_summary("Fastest read format", fastest_read, _read_mbps),
        _format_fastest_summary("Fastest write format", fastest_write, _write_mbps),
        _format_fastest_summary("Best balanced format", best_balanced, _balanced_mbps, unit_suffix="MB/s harmonic mean"),
    ]


def _best_available(rows: list[BenchResult], score: Callable[[BenchResult], float]) -> Optional[BenchResult]:
    # Deterministic tie-break by name to keep report summaries stable.
    return min(rows, key=lambda r: (-score(r), r.name), default=None)


def _benchmark_jobs(root: Path) -> list[JobSpec]:
    """Return benchmark job declarations (name, output path, adapters)."""
    return [
        ("json", root / "vel.json", (_json_writer, _json_reader)),
        ("binary_f32", root / "vel.bin", (_bin_writer, _bin_reader)),
        ("npy", root / "vel.npy", (_npy_writer, _npy_reader)),
        ("parquet", root / "vel.parquet", _parquet_adapters()),
        ("duckdb", root / "vel.duckdb", _duckdb_adapters()),
        ("hdf5", root / "vel.h5", _hdf5_adapters()),
        ("zarr", root / "vel.zarr", _zarr_adapters()),
        ("adios2", root / "vel.bp", _adios2_adapters()),
        ("mdio", root / "vel.mdio", _mdio_adapters()),
    ]


def _append_ranking_section(
    lines: list[str],
    title: str,
    rows: list[BenchResult],
    score: Callable[[BenchResult], float],
) -> None:
    lines.append("")
    lines.append(title)
    if rows:
        for i, r in enumerate(rows, start=1):
            lines.append(f"{i}. `{r.name}` — {score(r):.1f} MB/s")
    else:
        lines.append("- n/a")


def _bench_one(name: str, data: np.ndarray, path: Path, writer: Writer, reader: Reader, iterations: int) -> BenchResult:
    write_times = []
    read_times = []

    for _ in range(iterations):
        start = time.perf_counter()
        writer(data, path)
        write_times.append(time.perf_counter() - start)

    size_mb = _mb(path)

    for _ in range(iterations):
        start = time.perf_counter()
        loaded = reader(path, data.shape)
        read_times.append(time.perf_counter() - start)
        if loaded.shape != data.shape:
            raise RuntimeError(f"shape mismatch for {name}: {loaded.shape} != {data.shape}")

    w = float(np.mean(write_times))
    r = float(np.mean(read_times))

    return BenchResult(
        name=name,
        available=True,
        file_size_mb=size_mb,
        write_s=w,
        read_s=r,
        write_mbps=_throughput(size_mb, w),
        read_mbps=_throughput(size_mb, r),
    )


def _json_writer(data: np.ndarray, path: Path) -> None:
    with path.open("w") as f:
        json.dump(data.tolist(), f)


def _json_reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
    with path.open() as f:
        out = np.array(json.load(f), dtype=np.float32)
    return out.reshape(shape)


def _bin_writer(data: np.ndarray, path: Path) -> None:
    data.astype(np.float32, copy=False).tofile(path)


def _bin_reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
    out = np.fromfile(path, dtype=np.float32)
    return out.reshape(shape)


def _npy_writer(data: np.ndarray, path: Path) -> None:
    np.save(path, data.astype(np.float32, copy=False))


def _npy_reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
    out = np.load(path, allow_pickle=False)
    return out.reshape(shape).astype(np.float32, copy=False)


def _opt(name: str):
    try:
        return __import__(name)
    except Exception:
        return None


def _parquet_adapters() -> Optional[tuple[Writer, Reader]]:
    pa = _opt("pyarrow")
    if pa is None:
        return None
    pq = _opt("pyarrow.parquet")
    if pq is None:
        return None

    import pyarrow as pa_mod
    import pyarrow.parquet as pq_mod

    def writer(data: np.ndarray, path: Path) -> None:
        flat = data.reshape(-1).astype(np.float32)
        table = pa_mod.table({"value": pa_mod.array(flat)})
        pq_mod.write_table(table, path)

    def reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
        table = pq_mod.read_table(path, columns=["value"])
        arr = table["value"].to_numpy(zero_copy_only=False).astype(np.float32)
        return arr.reshape(shape)

    return writer, reader


def _duckdb_adapters() -> Optional[tuple[Writer, Reader]]:
    duckdb = _opt("duckdb")
    if duckdb is None:
        return None

    def writer(data: np.ndarray, path: Path) -> None:
        import duckdb as ddb

        con = ddb.connect(str(path))
        con.execute("CREATE OR REPLACE TABLE velocity(idx INTEGER, value REAL)")
        flat = data.reshape(-1).astype(np.float32)
        con.executemany(
            "INSERT INTO velocity VALUES (?, ?)",
            [(int(i), float(v)) for i, v in enumerate(flat)],
        )
        con.close()

    def reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
        import duckdb as ddb

        con = ddb.connect(str(path), read_only=True)
        vals = con.execute("SELECT value FROM velocity ORDER BY idx").fetchall()
        con.close()
        arr = np.array([v[0] for v in vals], dtype=np.float32)
        return arr.reshape(shape)

    return writer, reader


def _hdf5_adapters() -> Optional[tuple[Writer, Reader]]:
    h5py = _opt("h5py")
    if h5py is None:
        return None

    import h5py as h5

    def writer(data: np.ndarray, path: Path) -> None:
        with h5.File(path, "w") as f:
            f.create_dataset("velocity", data=data.astype(np.float32), compression=None)

    def reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
        with h5.File(path, "r") as f:
            out = np.array(f["velocity"], dtype=np.float32)
        return out.reshape(shape)

    return writer, reader


def _zarr_adapters() -> Optional[tuple[Writer, Reader]]:
    zarr = _opt("zarr")
    if zarr is None:
        return None

    import zarr as za

    def writer(data: np.ndarray, path: Path) -> None:
        za.save_array(path, data.astype(np.float32), overwrite=True)

    def reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
        out = np.array(za.load(path), dtype=np.float32)
        return out.reshape(shape)

    return writer, reader


def _adios2_adapters() -> Optional[tuple[Writer, Reader]]:
    adios2 = _opt("adios2")
    if adios2 is None:
        return None

    import adios2 as a2

    def writer(data: np.ndarray, path: Path) -> None:
        with a2.open(str(path), "w") as fh:
            fh.write("velocity", data.astype(np.float32))

    def reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
        with a2.open(str(path), "r") as fh:
            out = fh.read("velocity")
        return np.array(out, dtype=np.float32).reshape(shape)

    return writer, reader


def _mdio_adapters() -> Optional[tuple[Writer, Reader]]:
    mdio = _opt("mdio")
    if mdio is None:
        return None

    # MDIO API varies by version; keep best-effort and explicit if unsupported.
    def writer(data: np.ndarray, path: Path) -> None:
        raise RuntimeError("mdio adapter placeholder: API integration required for local mdio version")

    def reader(path: Path, shape: tuple[int, ...]) -> np.ndarray:
        raise RuntimeError("mdio adapter placeholder: API integration required for local mdio version")

    return writer, reader


def _generate_input_array(nx: int, nz: int, seed: int) -> np.ndarray:
    return np.random.default_rng(seed).random((nz, nx), dtype=np.float32)


def run_benchmark(nx: int, nz: int, iterations: int, seed: int = 0) -> list[BenchResult]:
    if nx <= 0 or nz <= 0:
        raise ValueError("nx and nz must be > 0")
    if iterations <= 0:
        raise ValueError("iterations must be > 0")

    data = _generate_input_array(nx, nz, seed)

    with tempfile.TemporaryDirectory(prefix="rtm3d_iofmt_") as td:
        root = Path(td)

        jobs = _benchmark_jobs(root)

        out: list[BenchResult] = []
        for name, path, adapters in jobs:
            if adapters is None:
                out.append(BenchResult(name=name, available=False, error="dependency unavailable"))
                continue
            writer, reader = adapters
            try:
                out.append(_bench_one(name, data, path, writer, reader, iterations))
            except Exception as exc:
                out.append(BenchResult(name=name, available=False, error=str(exc)))

        return out


def to_markdown(results: list[BenchResult], nx: int, nz: int, iterations: int, seed: int) -> str:
    available_count, unavailable_count, available_pct = _availability_stats(results)

    available = _available_rows(results)
    fastest_read = _best_available(available, _read_mbps)
    fastest_write = _best_available(available, _write_mbps)
    best_balanced = _best_available(available, _balanced_mbps)
    top_read = _rank_available(results, key=_read_mbps, top_n=_TOP_K)
    top_write = _rank_available(results, key=_write_mbps, top_n=_TOP_K)
    top_balanced = _rank_available(results, key=_balanced_mbps, top_n=_TOP_K)

    lines = [
        "# I/O Format Benchmark Report",
        "",
        *_report_metadata_lines(
            nx,
            nz,
            iterations,
            seed,
            available_count,
            unavailable_count,
            available_pct,
            fastest_read,
            fastest_write,
            best_balanced,
        ),
        "",
        *_TABLE_HEADER_LINES,
    ]

    for r in results:
        if r.available:
            lines.append(
                f"| {r.name} | ok | {r.file_size_mb:.2f} | {r.write_s*1000:.1f} | {r.read_s*1000:.1f} | {r.write_mbps:.1f} | {r.read_mbps:.1f} | - |"
            )
        else:
            lines.append(f"| {r.name} | n/a | - | - | - | - | - | {r.error} |")

    _append_ranking_section(
        lines,
        "## Top Read Throughput (available formats)",
        top_read,
        _read_mbps,
    )
    _append_ranking_section(
        lines,
        "## Top Write Throughput (available formats)",
        top_write,
        _write_mbps,
    )
    _append_ranking_section(
        lines,
        "## Top Balanced Throughput (harmonic mean of read/write)",
        top_balanced,
        _balanced_mbps,
    )

    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Benchmark multiple I/O formats for rtm3d arrays")
    ap.add_argument("--nx", type=int, default=400)
    ap.add_argument("--nz", type=int, default=300)
    ap.add_argument("--iterations", type=int, default=3)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--report", type=Path, default=Path("artifacts/io_format_benchmark.md"))
    args = ap.parse_args()

    results = run_benchmark(args.nx, args.nz, args.iterations, seed=args.seed)
    md = to_markdown(results, args.nx, args.nz, args.iterations, args.seed)

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(md)

    print(md)
    print(f"\nWrote report: {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
