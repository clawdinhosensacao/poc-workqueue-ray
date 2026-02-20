# RFC: Work Queue vs Ray for C++ HPC Bag-of-Tasks (zfp)

- Date: 2026-02-20
- Status: PoC v2 (organized build/run, executed)

## 1) Objective

Evaluate **CCTools Work Queue** vs **Ray** as task distribution layers for a heavy **C++** workload.

Hard constraints applied in this version:

- same task binary in both systems (`compress_task`)
- same codec in both systems (`zfp`)
- Ray used only for orchestration/distribution

## 2) Implementation summary

- `compress_task` (`src/compress_task.cpp`): generates random `float32` values, compresses with zfp, writes binary output, prints metrics.
- `workqueue_manager` (`src/workqueue_manager.cpp`): submits CLI tasks to workers.
- `ray_bag.py` (`src/ray_bag.py`): Ray remote wrapper launching the same C++ binary via subprocess.
- Build now standardized with **CMake** (`CMakeLists.txt`) and `FetchContent` for zfp.

## 3) Run configuration used

- tasks: 12
- n: 200000 elements
- parallelism: 3 workers/CPUs
- tolerance: 1e-3

Commands:

```bash
./scripts/build.sh
WQ_PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
TASKS=12 N=200000 CPUS=3 TOLERANCE=1e-3 ./scripts/run_ray.sh
python3 scripts/summarize_results.py
```

## 4) Measured result snapshot

Latest baseline from `results/benchmark_summary.txt` (24 tasks, local emulation):

- Work Queue: `wall_s=9.08486`, `tasks_per_s=2.64176`
- Ray: `wall_s=1.426`, `tasks_per_s=16.827`
- Overhead ratio (WQ/Ray wall): `6.371`

Spot simulation from `results/spot_sim_summary.log` (worker kill + Ray injected failures/retries):

- Work Queue: completed with one worker preempted (`workqueue_exit=0`)
- Ray: completed with failure injection and retries (`ray_exit=0`)

Interpretation: in these local runs, Ray showed lower orchestration overhead for this task size, while both stacks completed under synthetic preemption. This is still not a full production proxy (single host, no real cloud revocation events, no long soak).

## 5) Adoption criteria (team decision view)

### Robustness

- **Work Queue**: very good under worker churn/opportunistic pools; straightforward retry/reassignment model.
- **Ray**: robust when cluster lifecycle is well-managed; resilience quality depends more on runtime/autoscaler tuning.

### Maturity

- **Work Queue**: mature in HTC/scientific batch environments.
- **Ray**: mature and very active in data/ML/distributed Python ecosystems.

### Ease of programming

- **Work Queue**: lower-level, explicit task/file handling; good fit for binary-first HPC pipelines.
- **Ray**: better ergonomics for Python-heavy teams; concise parallel APIs.

### Operability

- **Work Queue**: simple process model; easier mental model in binary-centric workloads.
- **Ray**: richer platform but more moving parts (object store, scheduler, cluster services).

### Observability

- **Work Queue**: clear logs/task outputs; advanced telemetry is mostly DIY.
- **Ray**: stronger built-in observability surface, but requires Ray literacy.

### Spot/preemptible suitability

- **Work Queue**: naturally aligned with volatile workers; interruption handling is an idiomatic use-case.
- **Ray**: viable on spot, but demands disciplined retry/checkpoint/autoscaling settings.

## 6) On-prem SLURM + spot simulation plan

Implemented and executable today:

- `scripts/run_slurm_sim.sh`: uses real `sbatch/srun` if available, otherwise local emulation.
- `scripts/run_spot_sim.sh`: simulates preemption:
  - Work Queue: kills one worker mid-run.
  - Ray: injects random failures with retries.

This gives early signal now; real infrastructure validation should follow when budget/time are available.

## 7) Recommendation

Given your context (C++ heavy HPC tasks, team evaluation focus):

1. Keep this PoC as neutral baseline.
2. Run both scenario scripts repeatedly (different task sizes/concurrency).
3. If your team favors operational simplicity and volatile capacity handling, Work Queue remains a strong candidate.
4. If your team values developer velocity and can absorb runtime complexity, Ray becomes attractive.

The decision should weight failure behavior and operability at least as much as raw throughput from one benchmark.
