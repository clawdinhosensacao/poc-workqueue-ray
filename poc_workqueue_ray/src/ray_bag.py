#!/usr/bin/env python3
import argparse
import time
import numpy as np
import ray
import zfpy

@ray.remote
def compress_task(seed: int, n: int, tolerance: float = 1e-3):
    rng = np.random.default_rng(seed)
    arr = rng.uniform(-1000.0, 1000.0, size=n).astype(np.float32)
    t0 = time.perf_counter()
    compressed = zfpy.compress_numpy(arr, tolerance=tolerance)
    t1 = time.perf_counter()
    return {
        "seed": seed,
        "n": int(n),
        "input_bytes": int(arr.nbytes),
        "compressed_bytes": int(len(compressed)),
        "ratio": float(arr.nbytes / len(compressed)),
        "comp_ms": (t1 - t0) * 1000.0,
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--tasks", type=int, default=40)
    p.add_argument("--n", type=int, default=1_000_000)
    p.add_argument("--cpus", type=int, default=4)
    args = p.parse_args()

    ray.init(num_cpus=args.cpus, include_dashboard=False, logging_level="ERROR")

    t0 = time.perf_counter()
    refs = [compress_task.remote(1000 + i, args.n) for i in range(args.tasks)]
    results = ray.get(refs)
    t1 = time.perf_counter()

    for r in results:
        print(
            f"seed={r['seed']} n={r['n']} input_bytes={r['input_bytes']} "
            f"compressed_bytes={r['compressed_bytes']} ratio={r['ratio']:.4f} comp_ms={r['comp_ms']:.3f}"
        )

    wall_s = t1 - t0
    print(
        f"ray_summary tasks={args.tasks} done={len(results)} failed=0 wall_s={wall_s:.3f} tasks_per_s={len(results)/wall_s:.3f}"
    )

    ray.shutdown()


if __name__ == "__main__":
    main()
