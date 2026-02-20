#!/usr/bin/env python3
import argparse
import os
import random
import subprocess
import time

import ray


def parse_kv(line: str):
    out = {}
    for token in line.strip().split():
        if "=" not in token:
            continue
        k, v = token.split("=", 1)
        out[k] = v
    return out


def run_cpp_task(binary: str, seed: int, n: int, tolerance: float, out_dir: str, fail_prob: float):
    # Synthetic preemption/failure injection for spot simulation.
    if fail_prob > 0 and random.random() < fail_prob:
        raise RuntimeError("simulated_preemption")

    out_file = os.path.join(out_dir, f"ray_out_{seed}.bin")
    cmd = [
        binary,
        "--seed", str(seed),
        "--n", str(n),
        "--tolerance", str(tolerance),
        "--out", out_file,
    ]

    started = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    finished = time.perf_counter()

    if proc.returncode != 0:
        return {
            "seed": seed,
            "failed": True,
            "returncode": proc.returncode,
            "stderr": proc.stderr.strip(),
            "stdout": proc.stdout.strip(),
            "dispatch_ms": (finished - started) * 1000.0,
        }

    line = proc.stdout.strip().splitlines()[-1]
    data = parse_kv(line)
    data["seed"] = int(data.get("seed", seed))
    data["n"] = int(data["n"])
    data["input_bytes"] = int(data["input_bytes"])
    data["compressed_bytes"] = int(data["compressed_bytes"])
    data["ratio"] = float(data["ratio"])
    data["comp_ms"] = float(data["comp_ms"])
    data["total_ms"] = float(data["total_ms"])
    data["dispatch_ms"] = (finished - started) * 1000.0
    data["failed"] = False
    return data


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--tasks", type=int, default=40)
    p.add_argument("--n", type=int, default=1_000_000)
    p.add_argument("--cpus", type=int, default=4)
    p.add_argument("--tolerance", type=float, default=1e-3)
    p.add_argument("--binary", default=None)
    p.add_argument("--max-retries", type=int, default=0)
    p.add_argument("--fail-prob", type=float, default=0.0)
    args = p.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    binary = args.binary or os.path.join(root, "build", "compress_task")
    out_dir = os.path.join(root, "results")
    os.makedirs(out_dir, exist_ok=True)

    ray.init(num_cpus=args.cpus, include_dashboard=False, logging_level="ERROR")

    remote_fn = ray.remote(run_cpp_task)

    t0 = time.perf_counter()
    refs = [
        remote_fn.options(max_retries=args.max_retries, retry_exceptions=True).remote(
            binary, 1000 + i, args.n, args.tolerance, out_dir, args.fail_prob
        )
        for i in range(args.tasks)
    ]
    results = ray.get(refs)
    t1 = time.perf_counter()

    failed = 0
    for r in results:
        if r.get("failed"):
            failed += 1
            print(f"task_fail seed={r['seed']} rc={r['returncode']} stderr={r.get('stderr','')}")
        else:
            print(
                f"seed={r['seed']} n={r['n']} input_bytes={r['input_bytes']} "
                f"compressed_bytes={r['compressed_bytes']} ratio={r['ratio']:.4f} "
                f"comp_ms={r['comp_ms']:.3f} total_ms={r['total_ms']:.3f} dispatch_ms={r['dispatch_ms']:.3f}"
            )

    wall_s = t1 - t0
    done = len(results) - failed
    print(
        f"ray_summary tasks={args.tasks} done={done} failed={failed} "
        f"wall_s={wall_s:.3f} tasks_per_s={done / wall_s:.3f}"
    )

    ray.shutdown()


if __name__ == "__main__":
    main()
