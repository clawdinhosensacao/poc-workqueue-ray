# Scenario guide

## 1) On-premises SLURM cluster

Goal: run the PoC with resource allocation model similar to production HPC.

## Recommended mapping

- **Work Queue manager**: one control job (`sbatch` single task)
- **Work Queue workers**: SLURM array job (`--array`) or `srun` fan-out workers
- **Ray**: one head node allocation + workers as SLURM tasks/nodes

## Minimal workflow (when SLURM exists)

1. Build artifacts once on shared filesystem:

```bash
./scripts/build.sh
```

2. Submit scenario runner:

```bash
./scripts/run_slurm_sim.sh
```

The script auto-detects `sbatch/srun`; otherwise it emulates locally.

## SLURM-specific concerns to validate later

- Queue wait time and startup overhead under contention
- cgroup memory limits (Ray object store sizing)
- task placement / NUMA pinning for C++ binary consistency
- central log collection (`results/` on shared path)

## 2) Cloud spot/preemptible instances (simulated)

Budget is not available now, so we use local failure simulation.

## Current simulation model

Run:

```bash
./scripts/run_spot_sim.sh
```

It performs:

- **Work Queue preemption simulation**: kill one worker mid-run
- **Ray preemption simulation**: inject random task failures with retries

Outputs:

- `results/spot_workqueue_manager.log`
- `results/spot_workqueue_workers.log`
- `results/spot_ray.log`
- `results/spot_sim_summary.log`

## Future cloud validation checklist

When budget opens, validate on real spot nodes:

- interruption/revocation events and recovery time
- throughput degradation vs on-demand baseline
- cost per completed task
- checkpointing/retry policy impact
- autoscaling stabilization (Ray) vs worker churn handling (Work Queue)
