# RFC: PoC Comparison — CCTools Work Queue vs Ray with Shared C++ zfp Task

- **Author:** OpenClaw assistant
- **Date:** 2026-02-19
- **Status:** Draft after rerun

## 1) Decision scope

We need a task-distribution platform for bag-of-tasks workloads where task execution logic is in C++.

This revision enforces parity:

- **Same C++ executable (`compress_task`) in both systems**
- **Same codec (zfp) in both systems**
- Ray is used only for scheduling/distribution

## 2) PoC implementation and reproducibility

### 2.1 Workload

Per task:

- generate `n` random `float32` values
- compress with zfp (`accuracy/tolerance=1e-3`)
- write compressed output file
- print task metrics

Run params used for current measurements:

- `tasks=12`
- `n=200000`
- parallelism: `3` workers / CPU slots

### 2.2 Systems under test

1. **Work Queue path**
   - C++ manager submits command-line tasks to workers
   - workers execute `./compress_task`
2. **Ray path**
   - Python Ray remote function launches `bin/compress_task` via `subprocess`
   - task body remains C++

### 2.3 Local zfp build (no root)

`./scripts/install_zfp.sh`:

- clones `LLNL/zfp` tag `1.0.1`
- if `cmake` is missing, downloads local Kitware cmake binary into `third_party/tools`
- configures/builds/installs zfp into `third_party/zfp-install`

This addresses the prior blocker and keeps setup reproducible without system package installs.

## 3) Results (rerun)

From logs in `results/`:

- **Work Queue:** `wq_summary tasks=12 done=12 failed=0 wall_s=2.26823 tasks_per_s=5.29048`
- **Ray:** `ray_summary tasks=12 done=12 failed=0 wall_s=1.272 tasks_per_s=9.437`
- **Overhead eval (wall ratio):** `2.26823 / 1.272 = 1.783`

Interpretation:

- In this local single-host run, Ray completed faster for this task size.
- Because task kernel is now shared C++, this delta is more attributable to orchestration/runtime overhead than codec differences.
- Still not a full production proxy (no churn/fault injection, no multi-node network effects).

## 4) Adoption criteria evaluation

### 4.1 Robustness

- **Work Queue:** Strong in volatile/disconnected worker environments; mature retry/reassignment model for HTC-style campaigns.
- **Ray:** Robust in managed cluster settings, but operational robustness under heavy node churn depends more on Ray cluster lifecycle discipline.

### 4.2 Maturity

- **Work Queue:** Long history in scientific HTC ecosystems.
- **Ray:** Very active and broadly adopted in data/ML ecosystems; fast-moving platform.

### 4.3 Ease of programming

- **Work Queue:** Best for command/binary-centric workloads; explicit but lower-level.
- **Ray:** Better developer ergonomics for Python teams; concise APIs.

### 4.4 Operability

- **Work Queue:** Simple process model (manager/workers), explicit file staging, predictable CLI tooling.
- **Ray:** More moving parts (cluster services/object store/scheduler), but richer integrated platform features.

### 4.5 Observability

- **Work Queue:** Logs and task metadata are straightforward; custom metrics are DIY.
- **Ray:** Better built-in observability primitives/dashboards, but can require Ray expertise to interpret deeply.

### 4.6 Spot/preemptible suitability (cloud)

- **Work Queue:** Naturally aligned with opportunistic/preemptible pools; task retry model maps well.
- **Ray:** Can run on spot/preemptible nodes, but resilience depends more on autoscaler/config tuning and workload checkpointing patterns.

## 5) Recommendation (intellectually honest)

### Recommended default for this team (current constraints)

If near-term goal is a **C++ binary bag-of-tasks pipeline** with predictable operations and tolerance to worker volatility, choose **Work Queue** first.

### When to prefer Ray

Choose **Ray** when:

- team primarily develops in Python,
- richer integrated runtime features are needed,
- and the team can absorb operational complexity.

### Why this is honest

- Ray won this local throughput run.
- But platform choice should not be based on one local benchmark only.
- Operational fit and failure behavior under real pool conditions often dominate long-run outcomes.

## 6) Migration path

1. **Phase 0 (now):** keep shared C++ task binary and input/output contract stable.
2. **Phase 1:** productionize on Work Queue for binary-first workloads; add fault-injection tests (worker kill/preemption) and SLOs.
3. **Phase 2:** add Ray adapter in parallel (already started in this PoC via subprocess launcher).
4. **Phase 3:** if Python-centric orchestration benefits become decisive, migrate selected queues to Ray while preserving C++ task ABI.
5. **Phase 4:** periodically rerun parity benchmark with larger scale and cloud preemptible nodes.

## 7) Remaining limitations

- Single-host benchmark only
- Small task count
- No explicit cost model (compute + operations)
- No long-duration soak with repeated preemptions

## 8) Repro commands

```bash
./scripts/build.sh
PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
TASKS=12 N=200000 CPUS=3 TOLERANCE=1e-3 ./scripts/run_ray.sh
python3 scripts/summarize_results.py
```
