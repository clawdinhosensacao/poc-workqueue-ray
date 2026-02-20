# PoC: CCTools Work Queue vs Ray for Bag-of-Tasks (C++ compression workload)

This PoC compares two task-distribution approaches on the same host:

- **CCTools Work Queue** (manager + workers), task payload in **C++** using **zlib** fallback.
- **Ray** (Python API), task payload using **zfpy (zfp)**.

> Why zlib fallback on Work Queue side?
> We targeted zfp, but C++ zfp headers/libs were not available in this runtime without extra build steps. To keep the PoC runnable end-to-end now, Work Queue tasks use zlib compression in C++ and this is documented in the RFC as a comparability caveat.

## Layout

- `src/compress_task.cpp` — standalone C++ task binary (random float generation + zlib compression).
- `src/workqueue_manager.cpp` — C++ Work Queue manager that submits bag-of-tasks.
- `src/ray_bag.py` — Ray bag-of-tasks driver using zfpy compression.
- `scripts/build.sh` — compiles task binary + Work Queue manager.
- `scripts/run_workqueue.sh` — runs manager and local workers.
- `scripts/run_ray.sh` — runs Ray experiment.
- `results/` — logs from executions.
- `docs/RFC-workqueue-vs-ray.md` — findings and analysis.

## Prereqs used in this run

- Python 3.13.5
- Ray 2.54.0
- numpy 2.4.2
- zfpy 1.0.1
- CCTools Work Queue worker version 8.0.0 DEVELOPMENT (local source build)
- g++ 14.2.0

## Build

```bash
cd poc_workqueue_ray
./scripts/build.sh
```

## Run (example used for reported results)

```bash
# Work Queue
PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh

# Ray
TASKS=12 N=200000 CPUS=3 ./scripts/run_ray.sh
```

## Result logs

- `results/workqueue_manager.log`
- `results/workqueue_workers.log`
- `results/ray.log`

See RFC for interpretation and trade-offs.
