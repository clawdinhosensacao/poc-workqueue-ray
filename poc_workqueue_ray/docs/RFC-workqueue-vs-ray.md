# RFC: PoC Comparison — CCTools Work Queue vs Ray for Bag-of-Tasks

- **Author:** OpenClaw assistant
- **Date:** 2026-02-19
- **Status:** Draft (PoC completed and runnable)

## 1) Goal

Compare two solutions for distributing an embarrassingly parallel bag-of-tasks workload:

1. **CCTools Work Queue**
2. **Ray**

Target workload idea was compressing random arrays with zfp. In this environment:

- Ray path uses **zfpy (zfp)** successfully.
- Work Queue path uses a **C++ zlib fallback** (zfp C/C++ dev libs not readily available in runtime).

This means orchestration comparison is valid, but codec-level performance is not apples-to-apples.

## 2) Workload and setup

### Common shape

- `tasks=12`
- each task generates random `float32` array of `n=200000` (800,000 bytes input)
- execution on one machine with `3` workers / CPU slots

### Implementations

#### A. Work Queue

- Manager: `src/workqueue_manager.cpp`
- Worker payload binary: `src/compress_task.cpp` (C++ + zlib)
- Runtime command:

```bash
PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
```

#### B. Ray

- Driver and task function: `src/ray_bag.py`
- Compression: `zfpy.compress_numpy(..., tolerance=1e-3)`
- Runtime command:

```bash
TASKS=12 N=200000 CPUS=3 ./scripts/run_ray.sh
```

## 3) Observed results

From logs:

- **Work Queue:** `wq_summary tasks=12 done=12 failed=0 wall_s=4.49963 tasks_per_s=2.66689`
- **Ray:** `ray_summary tasks=12 done=12 failed=0 wall_s=1.183 tasks_per_s=10.140`

### Quick interpretation

- For this local run, Ray achieved higher throughput and lower wall time.
- But the task payloads use different codecs and serialization paths:
  - Work Queue: external process execution + file staging + zlib
  - Ray: in-process Python remote functions + object store + zfp
- Therefore, absolute speed delta should not be over-generalized as “Ray always faster.”

## 4) Strengths and weaknesses (intellectually honest)

## Work Queue (CCTools)

### Pros

- Excellent fit for **HTC-style** large distributed pools, opportunistic workers, heterogeneous environments.
- Simple mental model: manager submits shell commands; workers execute.
- Very explicit control over file transfers/caching/sandboxes.
- Good resilience model for long-running, failure-prone distributed campaigns.

### Cons

- Higher friction to bootstrap in this environment (building CCTools, wiring binaries/libs).
- More ceremony for interactive/iterative development loops.
- Process + file staging overhead can dominate for very small tasks.

## Ray

### Pros

- Very fast to prototype with Python API.
- Strong developer ergonomics for function/task abstractions.
- Good local-to-cluster portability for Python-heavy data workloads.
- Low overhead for many in-memory task patterns.

### Cons

- Best experience is Python-centric; integrating pure C++ pipelines may require extra glue.
- Runtime behavior can feel more opaque (autoscaling/object store internals) unless team is Ray-native.
- Dependency footprint can be heavier.

## 5) Recommendation by scenario

- Choose **Work Queue** when:
  - workload is command-line/binary oriented,
  - you need robust execution across volatile worker pools,
  - explicit file/sandbox control matters more than dev velocity.

- Choose **Ray** when:
  - team works primarily in Python,
  - iterative development speed matters,
  - tasks are in-memory and fine-grained.

## 6) Limitations of this PoC

1. **Codec mismatch** (zlib vs zfp) weakens direct performance comparison.
2. Single-host run; no WAN / multi-node stress.
3. No fault-injection benchmark (worker churn, retries, stragglers).
4. No cost model (CPU-hours, memory footprint, ops effort).

## 7) Next steps for fairer benchmark

1. Make both paths use exactly the same compression kernel:
   - either zfp in both,
   - or zlib in both.
2. Add scale sweeps (`tasks`, array size, workers).
3. Measure startup overhead separately from steady-state throughput.
4. Add failure tests (kill workers, retry behavior).
5. Record CPU/memory and include operational complexity score.

## 8) Reproducibility

See `README.md` and scripts in `scripts/`.

Primary logs:

- `results/workqueue_manager.log`
- `results/workqueue_workers.log`
- `results/ray.log`
