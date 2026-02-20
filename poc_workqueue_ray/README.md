# PoC: CCTools Work Queue vs Ray (same C++ zfp compression task)

This PoC compares orchestration overhead using **the same C++ binary** in both systems:

- **Work Queue**: C++ manager dispatches `compress_task` to workers.
- **Ray**: Python only distributes tasks; each Ray task invokes the same `bin/compress_task` executable via subprocess.

Compression kernel is **zfp in C++** for both paths.

## Layout

- `src/compress_task.cpp` — standalone C++ zfp compression task.
- `src/workqueue_manager.cpp` — C++ Work Queue manager.
- `src/ray_bag.py` — Ray distributor that launches the C++ binary.
- `scripts/install_zfp.sh` — local (no-root) zfp source build and install.
- `scripts/build.sh` — builds zfp + binaries.
- `scripts/run_workqueue.sh` — runs Work Queue benchmark.
- `scripts/run_ray.sh` — runs Ray benchmark.
- `scripts/summarize_results.py` — extracts benchmark summaries + overhead ratio.
- `results/` — execution logs.
- `docs/RFC-workqueue-vs-ray.md` — technical analysis and recommendation.

## Prerequisites

- CMake + C++ compiler toolchain
- Python 3 + `ray` installed
- Local CCTools source tree at `../cctools` with Work Queue worker binary

## Build (reproducible, no root)

```bash
cd poc_workqueue_ray
./scripts/build.sh
```

This will clone/build/install zfp from source into `third_party/zfp-install`.

## Run (same parameters used in current results)

```bash
PORT=9233 TASKS=12 N=200000 WORKERS=3 ./scripts/run_workqueue.sh
TASKS=12 N=200000 CPUS=3 TOLERANCE=1e-3 ./scripts/run_ray.sh
python3 scripts/summarize_results.py
```

## Logs

- `results/workqueue_manager.log`
- `results/workqueue_workers.log`
- `results/ray.log`
