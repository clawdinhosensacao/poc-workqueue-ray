# PoC — Work Queue vs Ray for C++ Bag-of-Tasks (zfp)

This repository compares **CCTools Work Queue** and **Ray** for distributing the same heavy C++ task.

## What is fair in this revision

- Same C++ executable in both paths: `compress_task`
- Same compression codec in both paths: `zfp`
- Ray is used only for scheduling/distribution (task body remains C++)

## Project layout

- `CMakeLists.txt` — primary build system (CMake, no ad-hoc Makefile)
- `src/compress_task.cpp` — C++ zfp compression workload
- `src/workqueue_manager.cpp` — Work Queue manager (C++)
- `src/ray_bag.py` — Ray distributor invoking the same C++ binary
- `scripts/build.sh` — configure/build via CMake
- `scripts/run_workqueue.sh` — local Work Queue run
- `scripts/run_ray.sh` — local Ray run
- `scripts/run_slurm_sim.sh` — SLURM scenario (real SLURM if available, local emulation otherwise)
- `scripts/run_spot_sim.sh` — spot/preemption simulation with fault injection
- `scripts/summarize_results.py` — writes `results/benchmark_summary.txt`
- `docs/RFC-workqueue-vs-ray.md` — architectural + adoption analysis
- `docs/SCENARIOS.md` — on-prem SLURM and cloud spot simulation plan

## Prerequisites

- CMake >= 3.20
- C++ toolchain (g++)
- Python 3 + `ray`
- Local CCTools checkout at `../cctools` with Work Queue built (`libwork_queue.a`, `libdttools.a`, `work_queue_worker`)

Install Ray if needed:

```bash
python3 -m pip install --user --break-system-packages ray
```

## Build

```bash
./scripts/build.sh
```

> CMake fetches/builds zfp automatically via `FetchContent`.

## Baseline run

```bash
WQ_PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
TASKS=12 N=200000 CPUS=3 TOLERANCE=1e-3 ./scripts/run_ray.sh
python3 scripts/summarize_results.py
cat results/benchmark_summary.txt
```

## Scenario runs

### On-prem SLURM scenario

```bash
./scripts/run_slurm_sim.sh
```

- Uses `sbatch/srun` if present.
- Falls back to local emulation if SLURM is not installed.

### Spot/preemptible simulation

```bash
./scripts/run_spot_sim.sh
```

- Work Queue: kills one worker during execution.
- Ray: injects failure probability and retries (`MAX_RETRIES=2`, `FAIL_PROB=0.20`).

## Documentation

- Full comparison and recommendation: `docs/RFC-workqueue-vs-ray.md`
- Deployment/scenario plan: `docs/SCENARIOS.md`
