#!/usr/bin/env python3
"""Benchmark I/O performance for rtm3d-cli.

Measures:
1. JSON parsing speed for 1D/2D arrays
2. Binary float32 read/write speed
3. Comparison of JSON vs binary load times
"""

import json
import os
import sys
import time
from pathlib import Path

import numpy as np


def benchmark_json_1d(n: int, iterations: int = 3) -> dict:
    """Benchmark 1D JSON array parsing."""
    data = np.random.rand(n).tolist()
    path = f"/tmp/bench_1d_{n}.json"
    
    with open(path, "w") as f:
        json.dump(data, f)
    
    file_size_mb = os.path.getsize(path) / (1024 * 1024)
    
    times = []
    for _ in range(iterations):
        start = time.perf_counter()
        with open(path) as f:
            loaded = json.load(f)
        elapsed = time.perf_counter() - start
        times.append(elapsed)
    
    os.remove(path)
    
    return {
        "type": "json_1d",
        "n": n,
        "file_size_mb": file_size_mb,
        "avg_time_s": np.mean(times),
        "min_time_s": np.min(times),
        "throughput_mbps": file_size_mb / np.mean(times),
    }


def benchmark_json_2d(nx: int, nz: int, iterations: int = 3) -> dict:
    """Benchmark 2D JSON array parsing."""
    data = np.random.rand(nz, nx).tolist()
    path = f"/tmp/bench_2d_{nx}x{nz}.json"
    
    with open(path, "w") as f:
        json.dump(data, f)
    
    file_size_mb = os.path.getsize(path) / (1024 * 1024)
    
    times = []
    for _ in range(iterations):
        start = time.perf_counter()
        with open(path) as f:
            loaded = json.load(f)
        elapsed = time.perf_counter() - start
        times.append(elapsed)
    
    os.remove(path)
    
    return {
        "type": "json_2d",
        "nx": nx,
        "nz": nz,
        "file_size_mb": file_size_mb,
        "avg_time_s": np.mean(times),
        "min_time_s": np.min(times),
        "throughput_mbps": file_size_mb / np.mean(times),
    }


def benchmark_binary_float32(size: int, iterations: int = 3) -> dict:
    """Benchmark binary float32 read/write."""
    data = np.random.rand(size).astype(np.float32)
    path = f"/tmp/bench_float32_{size}.bin"
    
    # Write benchmark
    write_times = []
    for _ in range(iterations):
        start = time.perf_counter()
        data.tofile(path)
        elapsed = time.perf_counter() - start
        write_times.append(elapsed)
    
    file_size_mb = os.path.getsize(path) / (1024 * 1024)
    
    # Read benchmark
    read_times = []
    for _ in range(iterations):
        start = time.perf_counter()
        loaded = np.fromfile(path, dtype=np.float32)
        elapsed = time.perf_counter() - start
        read_times.append(elapsed)
    
    os.remove(path)
    
    return {
        "type": "binary_float32",
        "size": size,
        "file_size_mb": file_size_mb,
        "avg_write_s": np.mean(write_times),
        "min_write_s": np.min(write_times),
        "avg_read_s": np.mean(read_times),
        "min_read_s": np.min(read_times),
        "write_throughput_mbps": file_size_mb / np.mean(write_times),
        "read_throughput_mbps": file_size_mb / np.mean(read_times),
    }


def benchmark_comparison(nx: int, nz: int, iterations: int = 3) -> dict:
    """Compare JSON vs binary for same data."""
    data = np.random.rand(nz, nx).astype(np.float32)
    
    # JSON path
    json_path = f"/tmp/bench_compare_{nx}x{nz}.json"
    with open(json_path, "w") as f:
        json.dump(data.tolist(), f)
    json_size_mb = os.path.getsize(json_path) / (1024 * 1024)
    
    # Binary path
    bin_path = f"/tmp/bench_compare_{nx}x{nz}.bin"
    data.tofile(bin_path)
    bin_size_mb = os.path.getsize(bin_path) / (1024 * 1024)
    
    # JSON read times
    json_times = []
    for _ in range(iterations):
        start = time.perf_counter()
        with open(json_path) as f:
            loaded = json.load(f)
        elapsed = time.perf_counter() - start
        json_times.append(elapsed)
    
    # Binary read times
    bin_times = []
    for _ in range(iterations):
        start = time.perf_counter()
        loaded = np.fromfile(bin_path, dtype=np.float32)
        elapsed = time.perf_counter() - start
        bin_times.append(elapsed)
    
    os.remove(json_path)
    os.remove(bin_path)
    
    return {
        "type": "comparison",
        "nx": nx,
        "nz": nz,
        "json_size_mb": json_size_mb,
        "binary_size_mb": bin_size_mb,
        "size_ratio": json_size_mb / bin_size_mb,
        "json_avg_s": np.mean(json_times),
        "binary_avg_s": np.mean(bin_times),
        "speedup": np.mean(json_times) / np.mean(bin_times),
    }


def main():
    print("=" * 60)
    print("rtm3d-cli I/O Benchmark")
    print("=" * 60)
    
    # Test sizes matching typical RTM models
    sizes_1d = [100, 500, 1000]
    sizes_2d = [(50, 50), (100, 100), (200, 200), (400, 300)]
    sizes_binary = [10000, 100000, 1000000, 10000000]
    
    print("\n--- 1D JSON Array Loading ---")
    for n in sizes_1d:
        r = benchmark_json_1d(n)
        print(f"  n={n:5d}: {r['file_size_mb']:6.2f} MB in {r['avg_time_s']*1000:6.1f} ms ({r['throughput_mbps']:.1f} MB/s)")
    
    print("\n--- 2D JSON Array Loading ---")
    for nx, nz in sizes_2d:
        r = benchmark_json_2d(nx, nz)
        print(f"  {nx}x{nz}: {r['file_size_mb']:6.2f} MB in {r['avg_time_s']*1000:6.1f} ms ({r['throughput_mbps']:.1f} MB/s)")
    
    print("\n--- Binary Float32 Read/Write ---")
    for size in sizes_binary:
        r = benchmark_binary_float32(size)
        print(f"  {size:10,d}: {r['file_size_mb']:6.2f} MB | "
              f"write {r['avg_write_s']*1000:6.1f} ms ({r['write_throughput_mbps']:.0f} MB/s) | "
              f"read {r['avg_read_s']*1000:6.1f} ms ({r['read_throughput_mbps']:.0f} MB/s)")
    
    print("\n--- JSON vs Binary Comparison ---")
    compare_sizes = [(100, 100), (200, 200), (400, 300)]
    for nx, nz in compare_sizes:
        r = benchmark_comparison(nx, nz)
        print(f"  {nx}x{nz}: JSON {r['json_size_mb']:.2f} MB ({r['json_avg_s']*1000:.1f} ms) | "
              f"Binary {r['binary_size_mb']:.2f} MB ({r['binary_avg_s']*1000:.1f} ms) | "
              f"speedup {r['speedup']:.1f}x | size ratio {r['size_ratio']:.1f}x")
    
    print("\n" + "=" * 60)
    print("Summary: Binary I/O is significantly faster than JSON for large arrays")
    print("=" * 60)


if __name__ == "__main__":
    main()
