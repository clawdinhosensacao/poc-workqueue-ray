# RFC: Work Queue vs Ray for C++ HPC Bag-of-Tasks (zfp)

- Date: 2026-02-20
- Status: PoC v3 (clean repo + scenario reruns)

## 1) Objective

Evaluate **CCTools Work Queue** vs **Ray** as distribution layers for heavy **C++** bag-of-tasks workloads.

Parity constraints enforced:

- same task binary in both systems (`compress_task`)
- same compression backend in both systems (`zfp`)
- Ray used only for orchestration (Python wrapper), task kernel stays C++

## 2) Implementation

- `src/compress_task.cpp`: C++ task (random float32 generation + zfp compression + metrics)
- `src/workqueue_manager.cpp`: C++ manager that dispatches command tasks
- `src/ray_bag.py`: Ray remote launcher of the same C++ binary via subprocess
- Build: `CMakeLists.txt` + `scripts/build.sh`

## 3) Executed runs and current results

### Baseline (local)

From `results/benchmark_summary.txt`:

- Work Queue: `done=24 failed=0 wall_s=5.2002 tasks_per_s=4.61521`
- Ray: `done=24 failed=0 wall_s=2.299 tasks_per_s=10.440`
- Wall-time ratio (WQ/Ray): `2.262`

### SLURM scenario (emulated locally in this environment)

From `results/slurm_sim_summary.txt`:

- Scenario ran successfully end-to-end.
- No native SLURM binaries were available in this runtime, so script used local emulation path.

### Spot/preemptible simulation

From `results/spot_sim_summary.txt`:

- Work Queue: completed after synthetic worker preemption (`workqueue_exit=0`)
- Ray: completed with injected failures + retries (`ray_exit=0`)

Interpretation: both systems can complete under synthetic interruption in this PoC. Ray had lower orchestration overhead in these local runs.

## 4) Why these technologies are considered mature

### Work Queue maturity signals

- Part of the long-lived **CCTools** ecosystem used in scientific/HTC workflows.
- Stable C API + CLI model focused on manager/worker execution and retries.
- Operational model is simple and explicit (files, tasks, workers), which tends to age well in HPC environments.

### Ray maturity signals

- Widely adopted open-source distributed runtime for production data/ML and parallel workloads.
- Active ecosystem and maintained APIs for task/actor abstractions.
- Strong tooling around execution, scaling, and observability compared to many lightweight alternatives.

## 5) Team adoption criteria

### Robustness

- **Work Queue:** strong on volatile worker pools and retry/reassignment semantics.
- **Ray:** robust with proper cluster lifecycle discipline and retry/checkpoint policies.

### Ease of programming

- **Work Queue:** lower-level but clear for binary-first orchestration.
- **Ray:** higher developer productivity for teams comfortable with Python orchestration.

### Operability

- **Work Queue:** fewer moving parts, straightforward failure debugging.
- **Ray:** richer runtime, but higher operational complexity (object store/scheduler/runtime tuning).

### Observability

- **Work Queue:** clear logs/task-level outputs; advanced metrics often custom.
- **Ray:** richer built-in observability surface.

### Spot/preemptible suitability

- **Work Queue:** naturally aligned to worker churn.
- **Ray:** works well with retries/checkpoints and autoscaling tuning.

## 6) Recommendation

Given your case (heavy C++ HPC tasks, team decision focus):

1. Keep Work Queue and Ray both in the shortlist.
2. Use this repo as the neutral baseline (shared task kernel already enforced).
3. Run repeated scenario tests on your real infra (especially real SLURM + real preemption events).
4. Decide primarily on **operability under failure + team ergonomics**, not one synthetic throughput number.

## 7) Another technology that may fit this use case

A strong additional candidate is **HTCondor**:

- very mature for high-throughput batch workloads,
- excellent for opportunistic/preemptible resources,
- good scheduling/policy controls for large task farms.

For Python-centric science workflows, **Parsl** is also worth considering as an orchestration layer over schedulers like SLURM/HTCondor.

## 8) Repro commands

```bash
./scripts/build.sh
WQ_PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
TASKS=12 N=200000 CPUS=3 TOLERANCE=1e-3 ./scripts/run_ray.sh
python3 scripts/summarize_results.py
./scripts/run_slurm_sim.sh
./scripts/run_spot_sim.sh
```
