# PoC — Work Queue vs Ray for C++ Bag-of-Tasks (zfp)

This repository compares **CCTools Work Queue** and **Ray** for distributing the same heavy C++ task.

## Fairness constraints in this version

- same C++ executable in both paths: `compress_task`
- same codec in both paths: `zfp`
- Ray only handles orchestration/distribution

## Structure

- `CMakeLists.txt` — canonical build
- `src/compress_task.cpp` — C++ zfp workload
- `src/workqueue_manager.cpp` — Work Queue manager
- `src/ray_bag.py` — Ray wrapper invoking the same C++ binary
- `scripts/build.sh` — CMake configure/build
- `scripts/run_workqueue.sh` — baseline Work Queue run
- `scripts/run_ray.sh` — baseline Ray run
- `scripts/run_slurm_sim.sh` — SLURM scenario (real SLURM if available, else emulation)
- `scripts/run_spot_sim.sh` — spot/preemption simulation
- `scripts/summarize_results.py` — writes `results/benchmark_summary.txt`
- `docs/RFC-workqueue-vs-ray.md` — technical + adoption analysis
- `docs/SCENARIOS.md` — scenario playbook

## Prerequisites

- CMake >= 3.20
- C++ toolchain
- Python 3 + ray
- local CCTools tree at `../cctools` with Work Queue built

Install Ray if needed:

```bash
python3 -m pip install --user --break-system-packages ray
```

## Build

```bash
./scripts/build.sh
```

> If system `cmake` is missing, `scripts/build.sh` downloads a local CMake binary under `third_party/tools`.

## Baseline run

```bash
WQ_PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
TASKS=12 N=200000 CPUS=3 TOLERANCE=1e-3 ./scripts/run_ray.sh
python3 scripts/summarize_results.py
cat results/benchmark_summary.txt
```

## Scenario runs

### SLURM/on-prem scenario

```bash
./scripts/run_slurm_sim.sh
cat results/slurm_sim_summary.txt
```

### Spot/preemptible simulation

```bash
./scripts/run_spot_sim.sh
cat results/spot_sim_summary.txt
```

## About old `connection refused` messages

If you see old lines such as `couldn't connect ... Connection refused`, they usually came from previous failed attempts where workers started before a valid manager or with stale port values. For current runs, trust the latest summary files:

- `results/benchmark_summary.txt`
- `results/slurm_sim_summary.txt`
- `results/spot_sim_summary.txt`
