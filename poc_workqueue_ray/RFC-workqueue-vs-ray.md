# RFC: PoC Comparison — CCTools Work Queue vs Ray for Bag-of-Tasks Compression

Date: 2026-02-19 22:10 (America/Sao_Paulo)

## 1) Scope

Implemented and executed two equivalent bag-of-tasks benchmarks where each task:
- deterministically generates pseudo-random bytes
- compresses bytes using **zlib**
- records timing and compression ratio

> Note on codec: requested zfp was not available in this environment; used zlib fallback and documented here.

## 2) Environment Notes

- Host: Linux 6.8.0-94-generic x86_64
- Python: 3.13.5
- C++ compiler: g++
- CCTools: built from source at `../cctools` (this workspace)
- Ray: installed with `pip3 install --break-system-packages ray`
- zfp: not present (`zfpy` unavailable; no zfp library found)

## 3) Implementations

### A. Work Queue (CCTools)

Files:
- `compress_task.cpp` — C++ task payload binary using zlib
- `wq_manager.cpp` — C++ manager using Work Queue API
- `run_work_queue.sh` — orchestration script (manager + N workers)
- `Makefile` — builds `compress_task` and `wq_manager`

Architecture:
- Manager submits tasks to Work Queue on a fixed port.
- Local workers (`work_queue_worker`) connect to manager.
- Task executable (`compress_task`) sent as cached input file.
- Per-task outputs (`out_*.bin`, `stats_*.json`) returned to manager.

### B. Ray

Files:
- `ray_benchmark.py` — Python Ray remote-function benchmark
- `run_ray.sh` — wrapper to run and save logs/artifacts

Architecture:
- `ray.init(num_cpus=N)` on local node.
- Remote function generates bytes and compresses via Python `zlib`.
- Driver gathers results via `ray.get` and writes JSON output.

## 4) Executed Runs and Results

Workload parameters (both systems):
- tasks: 24
- bytes per task: 2,097,152 (2 MiB)
- workers/CPUs: 4
- zlib level: 6

### Work Queue run

Command:
```bash
./run_work_queue.sh 24 2097152 4 9123 test1
```

Artifacts:
- `runs/test1/work_queue/logs/manager.log`
- `runs/test1/work_queue/logs/worker_*.log`
- `runs/test1/work_queue/results/stats_*.json`
- `runs/test1/work_queue/summary.txt`

Observed summary:
- Manager wall time: **1957 ms**
- Completed: 24/24, failed: 0
- Task `total_ms` mean (from stats files): **120.08 ms**
- Task `p50`: 120 ms, `p95`: 148 ms
- Mean compression ratio: 1.00031 (expected for random data)

### Ray run

Command:
```bash
./run_ray.sh 24 2097152 4 test1
```

Artifacts:
- `runs/test1/ray/ray.log`
- `runs/test1/ray/ray_results.json`

Observed summary:
- Wall time: **3083.84 ms**
- Task `total_ms` mean: **319.33 ms**
- Task `p50`: 199.12 ms, `p95`: 604.56 ms
- Mean compression ratio: 1.000308

## 5) Interpretation (Intellectually Honest)

- In this **single-node, local-only** PoC, Work Queue finished faster wall-clock than Ray for this specific micro-workload.
- This does **not** imply Work Queue is universally faster than Ray. Likely contributors:
  - Ray startup/runtime overhead on first run (local cluster bootstrap)
  - Python task overhead vs native C++ payload binary
  - `/dev/shm` warning for Ray object store (degraded local performance)
- Compression ratio is identical (random input is incompressible), validating functional equivalence.

## 6) Dev Effort & Operational Tradeoffs

### Work Queue

Strengths:
- Clear manager/worker semantics for distributed batch workloads.
- Good fit when tasks are shell/C/C++ executables with explicit file staging.
- Strong control over task sandboxing and file movement.

Weaknesses:
- More manual orchestration (ports, workers, binaries, staging).
- More moving parts for local experimentation.
- API ergonomics lower than high-level Python frameworks.

### Ray

Strengths:
- Very low friction for Python users.
- Fast prototyping with `@ray.remote` and object references.
- Rich ecosystem for scaling beyond simple task farms.

Weaknesses:
- Runtime/bootstrap overhead can dominate tiny tasks.
- Requires awareness of object-store/memory environment tuning.
- Python overhead unless using compiled extensions.

## 7) Reproducible Run Instructions

From `poc_workqueue_ray/`:

1. Build CCTools Work Queue in sibling directory (already done in this PoC):
```bash
cd ../cctools
./configure --prefix=$PWD/build
make -j4 work_queue
cd ../poc_workqueue_ray
```

2. Install Ray:
```bash
pip3 install --break-system-packages ray
```

3. Build PoC binaries:
```bash
make all
```

4. Run Work Queue benchmark:
```bash
./run_work_queue.sh 24 2097152 4 9123 test1
```

5. Run Ray benchmark:
```bash
./run_ray.sh 24 2097152 4 test1
```

6. Inspect outputs under:
- `runs/test1/work_queue/`
- `runs/test1/ray/`

## 8) Recommendation

- If your workload is primarily executable-based bag-of-tasks with explicit files and heterogeneous workers, Work Queue is compelling.
- If your team is Python-centric and needs broader distributed-compute abstractions, Ray is usually faster to iterate.
- For fairer performance conclusions, next step is:
  - multiple repetitions
  - warm-start Ray and Work Queue separately
  - scale task size and cluster size
  - optionally test zfp once available.
