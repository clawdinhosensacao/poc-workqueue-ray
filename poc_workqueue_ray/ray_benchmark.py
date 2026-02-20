#!/usr/bin/env python3
import argparse
import json
import random
import statistics
import time
import zlib

import ray


@ray.remote
def compress_task(seed: int, nbytes: int, level: int):
    t0 = time.perf_counter()
    rng = random.Random(seed)
    payload = rng.randbytes(nbytes)
    t1 = time.perf_counter()
    out = zlib.compress(payload, level)
    t2 = time.perf_counter()
    return {
        "seed": seed,
        "input_bytes": nbytes,
        "compressed_bytes": len(out),
        "ratio": len(out) / nbytes,
        "gen_ms": (t1 - t0) * 1000,
        "comp_ms": (t2 - t1) * 1000,
        "total_ms": (t2 - t0) * 1000,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tasks", type=int, default=40)
    ap.add_argument("--bytes", type=int, default=4 * 1024 * 1024)
    ap.add_argument("--level", type=int, default=6)
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--out", default="ray_results.json")
    args = ap.parse_args()

    ray.init(num_cpus=args.workers, include_dashboard=False, log_to_driver=False)

    t0 = time.perf_counter()
    futures = [compress_task.remote(i, args.bytes, args.level) for i in range(args.tasks)]
    results = ray.get(futures)
    t1 = time.perf_counter()

    wall_ms = (t1 - t0) * 1000
    totals = [r["total_ms"] for r in results]
    ratios = [r["ratio"] for r in results]

    summary = {
        "tasks": args.tasks,
        "bytes_per_task": args.bytes,
        "workers": args.workers,
        "wall_ms": wall_ms,
        "task_total_ms_mean": statistics.mean(totals),
        "task_total_ms_p50": statistics.median(totals),
        "task_total_ms_p95": sorted(totals)[int(0.95 * (len(totals) - 1))],
        "ratio_mean": statistics.mean(ratios),
    }

    with open(args.out, "w", encoding="utf-8") as f:
        json.dump({"summary": summary, "results": results}, f, indent=2)

    print("SUMMARY", json.dumps(summary))
    ray.shutdown()


if __name__ == "__main__":
    main()
