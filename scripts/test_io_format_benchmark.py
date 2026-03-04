#!/usr/bin/env python3

import sys
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


if __name__ == "__main__":
    unittest.main()
