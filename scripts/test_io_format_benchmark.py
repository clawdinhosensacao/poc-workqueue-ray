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
        self.assertIn("| json | ok |", md)
        self.assertIn("| hdf5 | n/a |", md)

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


if __name__ == "__main__":
    unittest.main()
