#!/usr/bin/env python3

import sys
import tempfile
import unittest
from pathlib import Path

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

import numpy as np

import io_format_benchmark as b


class IoFormatBenchmarkTests(unittest.TestCase):
    def test_to_markdown_contains_rows(self):
        rows = [
            b.BenchResult(name="json", available=True, file_size_mb=1.0, write_s=0.1, read_s=0.2, write_mbps=10.0, read_mbps=5.0),
            b.BenchResult(name="hdf5", available=False, error="dependency unavailable"),
        ]
        md = b.to_markdown(rows, nx=10, nz=20, iterations=2, seed=123)
        self.assertIn("- Shape: `20 x 10` float32", md)
        self.assertIn("- Seed: `123`", md)
        self.assertIn("- Formats available: `1`", md)
        self.assertIn("- Formats unavailable: `1`", md)
        self.assertIn("- Availability ratio: `50.0%`", md)
        self.assertIn("- Fastest read format: `json` (5.0 MB/s)", md)
        self.assertIn("- Fastest write format: `json` (10.0 MB/s)", md)
        self.assertIn("- Best balanced format: `json` (6.7 MB/s harmonic mean)", md)
        self.assertIn("| json | ok |", md)
        self.assertIn("| hdf5 | n/a |", md)
        self.assertIn("## Top Read Throughput (available formats)", md)
        self.assertIn("1. `json` — 5.0 MB/s", md)
        self.assertIn("## Top Write Throughput (available formats)", md)
        self.assertIn("1. `json` — 10.0 MB/s", md)
        self.assertIn("## Top Balanced Throughput (harmonic mean of read/write)", md)
        self.assertIn("1. `json` — 6.7 MB/s", md)

    def test_core_formats_run(self):
        results = b.run_benchmark(nx=32, nz=16, iterations=1)
        by_name = {r.name: r for r in results}

        self.assertTrue(by_name["json"].available)
        self.assertTrue(by_name["binary_f32"].available)
        self.assertTrue(by_name["npy"].available)

        self.assertGreater(by_name["json"].file_size_mb, 0.0)
        self.assertGreater(by_name["binary_f32"].file_size_mb, 0.0)
        self.assertGreater(by_name["npy"].file_size_mb, 0.0)

    def test_result_set_includes_all_declared_formats(self):
        results = b.run_benchmark(nx=16, nz=8, iterations=1)
        names = {r.name for r in results}
        expected = {
            "json",
            "binary_f32",
            "npy",
            "parquet",
            "duckdb",
            "hdf5",
            "zarr",
            "adios2",
            "mdio",
        }
        self.assertEqual(names, expected)

    def test_benchmark_jobs_declares_expected_order(self):
        jobs = b._benchmark_jobs(Path("/tmp/example"))
        self.assertEqual(
            [name for name, _path, _adapters in jobs],
            ["json", "binary_f32", "npy", "parquet", "duckdb", "hdf5", "zarr", "adios2", "mdio"],
        )

    def test_benchmark_jobs_use_given_root_for_paths(self):
        root = Path("/tmp/example")
        jobs = b._benchmark_jobs(root)
        self.assertTrue(all(path.parent == root for _name, path, _adapters in jobs))

    def test_benchmark_jobs_expected_output_filenames(self):
        jobs = b._benchmark_jobs(Path("/tmp/example"))
        self.assertEqual(
            [path.name for _name, path, _adapters in jobs],
            ["vel.json", "vel.bin", "vel.npy", "vel.parquet", "vel.duckdb", "vel.h5", "vel.zarr", "vel.bp", "vel.mdio"],
        )

    def test_generate_input_array_is_seed_deterministic(self):
        a = b._generate_input_array(nx=8, nz=4, seed=42)
        c = b._generate_input_array(nx=8, nz=4, seed=42)
        d = b._generate_input_array(nx=8, nz=4, seed=43)

        self.assertTrue(np.array_equal(a, c))
        self.assertFalse(np.array_equal(a, d))

    def test_run_benchmark_rejects_invalid_dimensions_or_iterations(self):
        with self.assertRaises(ValueError):
            b.run_benchmark(nx=0, nz=8, iterations=1)
        with self.assertRaises(ValueError):
            b.run_benchmark(nx=8, nz=0, iterations=1)
        with self.assertRaises(ValueError):
            b.run_benchmark(nx=8, nz=8, iterations=0)

    def test_append_ranking_section_handles_empty_and_nonempty_rows(self):
        lines = []
        b._append_ranking_section(lines, "## Example", [], lambda r: r.read_mbps)
        self.assertEqual(lines, ["", "## Example", "- n/a"])

        rows = [b.BenchResult(name="bin", available=True, read_mbps=12.34, write_mbps=56.78)]
        b._append_ranking_section(lines, "## Example 2", rows, lambda r: r.read_mbps)
        self.assertEqual(lines[-3:], ["", "## Example 2", "1. `bin` — 12.3 MB/s"])

    def test_balanced_mbps_returns_zero_for_nonpositive_components(self):
        self.assertEqual(b._balanced_mbps(b.BenchResult(name="x", available=True, read_mbps=0.0, write_mbps=10.0)), 0.0)
        self.assertEqual(b._balanced_mbps(b.BenchResult(name="x", available=True, read_mbps=10.0, write_mbps=0.0)), 0.0)

    def test_rank_available_filters_unavailable_and_breaks_ties_by_name(self):
        rows = [
            b.BenchResult(name="zeta", available=True, read_mbps=10.0),
            b.BenchResult(name="alpha", available=True, read_mbps=10.0),
            b.BenchResult(name="beta", available=True, read_mbps=9.0),
            b.BenchResult(name="ghost", available=False, read_mbps=99.0),
        ]
        ranked = b._rank_available(rows, key=lambda r: r.read_mbps, top_n=3)
        self.assertEqual([r.name for r in ranked], ["alpha", "zeta", "beta"])

    def test_rank_available_respects_top_n_limit(self):
        rows = [
            b.BenchResult(name="a", available=True, read_mbps=4.0),
            b.BenchResult(name="b", available=True, read_mbps=3.0),
            b.BenchResult(name="c", available=True, read_mbps=2.0),
        ]
        ranked = b._rank_available(rows, key=lambda r: r.read_mbps, top_n=2)
        self.assertEqual([r.name for r in ranked], ["a", "b"])

    def test_rank_available_default_top_n_matches_module_constant(self):
        rows = [
            b.BenchResult(name="a", available=True, read_mbps=5.0),
            b.BenchResult(name="b", available=True, read_mbps=4.0),
            b.BenchResult(name="c", available=True, read_mbps=3.0),
            b.BenchResult(name="d", available=True, read_mbps=2.0),
        ]
        ranked = b._rank_available(rows, key=lambda r: r.read_mbps)
        self.assertEqual(len(ranked), b._TOP_K)

    def test_rank_available_empty_input_returns_empty_list(self):
        self.assertEqual(b._rank_available([], key=lambda r: r.read_mbps), [])

    def test_availability_stats_for_empty_and_mixed_rows(self):
        self.assertEqual(b._availability_stats([]), (0, 0, 0.0))

        rows = [
            b.BenchResult(name="json", available=True),
            b.BenchResult(name="npy", available=True),
            b.BenchResult(name="hdf5", available=False),
        ]
        avail, unavail, pct = b._availability_stats(rows)
        self.assertEqual((avail, unavail), (2, 1))
        self.assertAlmostEqual(pct, 66.6666667, places=5)

    def test_available_rows_filters_only_available_entries(self):
        rows = [
            b.BenchResult(name="json", available=True),
            b.BenchResult(name="npy", available=True),
            b.BenchResult(name="hdf5", available=False),
        ]
        self.assertEqual([r.name for r in b._available_rows(rows)], ["json", "npy"])

    def test_to_markdown_handles_no_available_formats(self):
        rows = [
            b.BenchResult(name="json", available=False, error="dependency unavailable"),
            b.BenchResult(name="npy", available=False, error="dependency unavailable"),
        ]
        md = b.to_markdown(rows, nx=10, nz=20, iterations=1, seed=0)
        self.assertIn("- Formats available: `0`", md)
        self.assertIn("- Formats unavailable: `2`", md)
        self.assertIn("- Availability ratio: `0.0%`", md)
        self.assertIn("- Fastest read format: `n/a`", md)
        self.assertIn("- Fastest write format: `n/a`", md)
        self.assertIn("- Best balanced format: `n/a`", md)
        self.assertIn("## Top Read Throughput (available formats)\n- n/a", md)
        self.assertIn("## Top Write Throughput (available formats)\n- n/a", md)
        self.assertIn("## Top Balanced Throughput (harmonic mean of read/write)\n- n/a", md)

    def test_format_fastest_summary_supports_custom_suffix(self):
        row = b.BenchResult(name="bin", available=True, read_mbps=12.34)
        self.assertEqual(
            b._format_fastest_summary("Fastest read format", row, lambda r: r.read_mbps),
            "- Fastest read format: `bin` (12.3 MB/s)",
        )
        rounded_up = b.BenchResult(name="bin", available=True, read_mbps=12.36)
        self.assertEqual(
            b._format_fastest_summary("Fastest read format", rounded_up, lambda r: r.read_mbps),
            "- Fastest read format: `bin` (12.4 MB/s)",
        )
        self.assertEqual(
            b._format_fastest_summary("Best balanced format", row, lambda r: r.read_mbps, unit_suffix="MB/s harmonic mean"),
            "- Best balanced format: `bin` (12.3 MB/s harmonic mean)",
        )
        self.assertEqual(b._format_fastest_summary("Fastest read format", None, lambda r: r.read_mbps), "- Fastest read format: `n/a`")

    def test_best_available_selects_highest_score_or_none(self):
        rows = [
            b.BenchResult(name="slow", available=True, read_mbps=1.0),
            b.BenchResult(name="fast", available=True, read_mbps=3.0),
            b.BenchResult(name="mid", available=True, read_mbps=2.0),
        ]
        best = b._best_available(rows, lambda r: r.read_mbps)
        self.assertIsNotNone(best)
        self.assertEqual(best.name, "fast")
        self.assertIsNone(b._best_available([], lambda r: r.read_mbps))

    def test_best_available_breaks_ties_by_name(self):
        rows = [
            b.BenchResult(name="zeta", available=True, read_mbps=7.0),
            b.BenchResult(name="alpha", available=True, read_mbps=7.0),
        ]
        best = b._best_available(rows, lambda r: r.read_mbps)
        self.assertIsNotNone(best)
        self.assertEqual(best.name, "alpha")

    def test_throughput_returns_zero_for_nonpositive_seconds(self):
        self.assertEqual(b._throughput(10.0, 0.0), 0.0)
        self.assertEqual(b._throughput(10.0, -1.0), 0.0)
        self.assertEqual(b._throughput(10.0, 2.0), 5.0)

    def test_read_write_score_helpers_return_expected_fields(self):
        row = b.BenchResult(name="fmt", available=True, read_mbps=3.25, write_mbps=7.5)
        self.assertEqual(b._read_mbps(row), 3.25)
        self.assertEqual(b._write_mbps(row), 7.5)

    def test_shape_label_uses_nz_by_nx_order(self):
        self.assertEqual(b._shape_label(nx=400, nz=300), "300 x 400")

    def test_append_ranking_section_rounds_to_single_decimal(self):
        lines = []
        rows = [b.BenchResult(name="fmt", available=True, read_mbps=1.04)]
        b._append_ranking_section(lines, "## Rounded", rows, lambda r: r.read_mbps)
        self.assertEqual(lines[-1], "1. `fmt` — 1.0 MB/s")

    def test_mb_converts_file_size_to_megabytes(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "size.bin"
            path.write_bytes(b"\x00" * 2048)
            self.assertAlmostEqual(b._mb(path), 2048 / (1024 * 1024), places=9)


if __name__ == "__main__":
    unittest.main()
