#!/usr/bin/env python3
"""E2E quality metrics for RTM migration results.

Outputs JSON metrics for CI integration.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path

import numpy as np


@dataclass
class MigrationMetrics:
    nx: int
    nz: int
    energy_normalized: float
    energy_total: float
    max_amplitude: float
    min_amplitude: float
    focus_ratio: float
    finite_ratio: float
    nonzero_ratio: float
    elapsed_ms: float = 0.0


def compute_metrics(image: np.ndarray, nx: int, nz: int, elapsed_ms: float = 0.0) -> MigrationMetrics:
    """Compute quality metrics for migrated image."""
    flat = image.ravel()
    n = flat.size

    finite_mask = np.isfinite(flat)
    finite_ratio = float(finite_mask.sum()) / n

    finite_vals = flat[finite_mask]
    if finite_vals.size == 0:
        return MigrationMetrics(
            nx=nx, nz=nz,
            energy_normalized=0.0, energy_total=0.0,
            max_amplitude=0.0, min_amplitude=0.0,
            focus_ratio=0.0, finite_ratio=0.0, nonzero_ratio=0.0,
            elapsed_ms=elapsed_ms
        )

    energy_total = float(np.sum(np.abs(finite_vals)))
    energy_normalized = float(np.mean(np.abs(finite_vals)))
    max_amp = float(np.max(np.abs(finite_vals)))
    min_amp = float(np.min(finite_vals))

    nonzero_ratio = float((np.abs(finite_vals) > 1e-10).sum()) / finite_vals.size

    # Focus: gradient energy ratio
    img2d = finite_vals.reshape(nz, nx)
    gx = np.abs(np.diff(img2d, axis=1))
    gz = np.abs(np.diff(img2d, axis=0))
    grad_energy = float(np.mean(gx)) + float(np.mean(gz))
    focus_ratio = grad_energy / (energy_normalized + 1e-10)

    return MigrationMetrics(
        nx=nx, nz=nz,
        energy_normalized=energy_normalized,
        energy_total=energy_total,
        max_amplitude=max_amp,
        min_amplitude=min_amp,
        focus_ratio=focus_ratio,
        finite_ratio=finite_ratio,
        nonzero_ratio=nonzero_ratio,
        elapsed_ms=elapsed_ms
    )


def check_thresholds(m: MigrationMetrics) -> list[str]:
    """Check metrics against thresholds, return list of failures."""
    failures = []

    if m.finite_ratio < 0.999:
        failures.append(f"finite_ratio {m.finite_ratio:.4f} < 0.999")

    if m.energy_normalized < 1e-6:
        failures.append(f"energy_normalized {m.energy_normalized:.2e} < 1e-6")

    if m.focus_ratio < 0.05:
        failures.append(f"focus_ratio {m.focus_ratio:.4f} < 0.05")

    if m.nonzero_ratio < 0.01:
        failures.append(f"nonzero_ratio {m.nonzero_ratio:.4f} < 0.01")

    if m.max_amplitude > 1e10:
        failures.append(f"max_amplitude {m.max_amplitude:.2e} suggests instability")

    return failures


def main() -> int:
    ap = argparse.ArgumentParser(description="Compute E2E quality metrics for RTM output")
    ap.add_argument("--input", required=True, help="Path to float32 raw image file")
    ap.add_argument("--meta", required=True, help="Path to JSON metadata file")
    ap.add_argument("--output", help="Path to output JSON metrics file")
    ap.add_argument("--fail-on-threshold", action="store_true",
                    help="Exit with error code if thresholds not met")
    args = ap.parse_args()

    input_path = Path(args.input)
    meta_path = Path(args.meta)

    if not input_path.exists():
        print(f"ERROR: input file not found: {input_path}", file=sys.stderr)
        return 1

    if not meta_path.exists():
        print(f"ERROR: meta file not found: {meta_path}", file=sys.stderr)
        return 1

    meta = json.loads(meta_path.read_text())
    nx = int(meta["nx"])
    nz = int(meta["nz"])

    image = np.fromfile(input_path, dtype="<f4")
    if image.size != nx * nz:
        print(f"ERROR: expected {nx * nz} values, got {image.size}", file=sys.stderr)
        return 1

    start = time.perf_counter()
    metrics = compute_metrics(image, nx, nz)
    metrics.elapsed_ms = (time.perf_counter() - start) * 1000.0

    failures = check_thresholds(metrics)

    output = {
        "metrics": asdict(metrics),
        "threshold_failures": failures,
        "status": "ok" if not failures else "fail"
    }

    out_json = json.dumps(output, indent=2)
    print(out_json)

    if args.output:
        Path(args.output).write_text(out_json)

    if args.fail_on_threshold and failures:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
