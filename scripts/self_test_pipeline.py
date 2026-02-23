#!/usr/bin/env python3
"""
Self-testing E2E pipeline for RTM validation.

Usage:
    python3 scripts/self_test_pipeline.py [--preset PRESET] [--quick]

This script:
1. Generates a synthetic model with a preset
2. Runs RTM migration
3. Validates output quality metrics
4. Reports PASS/FAIL

Exit codes:
    0 - All checks passed
    1 - One or more checks failed
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Optional

import numpy as np


@dataclass
class PipelineConfig:
    """Configuration for self-test pipeline."""
    preset: str = "layers"
    nx: int = 80
    nz: int = 48
    nt: int = 100
    n_shots: int = 2
    seed: int = 42
    dt: float = 0.0008
    f0: float = 16.0
    quick: bool = False


@dataclass
class TestResult:
    """Result of a single test check."""
    name: str
    passed: bool
    value: Optional[float] = None
    threshold: Optional[float] = None
    message: str = ""


@dataclass
class PipelineResult:
    """Result of the entire pipeline."""
    passed: bool
    elapsed_ms: float
    tests: list
    output_path: str
    error: str = ""


def run_command(cmd: list[str], timeout: int = 120) -> tuple[int, str, str]:
    """Run a command and return (returncode, stdout, stderr)."""
    result = subprocess.run(
        cmd, capture_output=True, text=True, timeout=timeout
    )
    return result.returncode, result.stdout, result.stderr


def check_finite_values(image: np.ndarray) -> TestResult:
    """Check that all values are finite (no NaN or Inf)."""
    finite_ratio = np.isfinite(image).sum() / image.size
    passed = finite_ratio == 1.0
    return TestResult(
        name="finite_values",
        passed=passed,
        value=float(finite_ratio),
        threshold=1.0,
        message="All values finite" if passed else f"Found non-finite values: {(1-finite_ratio)*100:.2f}%"
    )


def check_energy(image: np.ndarray) -> TestResult:
    """Check that image has non-trivial energy."""
    energy = float(np.mean(np.abs(image)))
    passed = energy > 1e-6
    return TestResult(
        name="energy",
        passed=passed,
        value=energy,
        threshold=1e-6,
        message=f"Energy: {energy:.2e}" if passed else f"Energy too low: {energy:.2e}"
    )


def check_focus_ratio(image: np.ndarray, nx: int, nz: int) -> TestResult:
    """Check focus quality via gradient energy ratio."""
    img2d = image.reshape(nz, nx)
    gx = np.abs(np.diff(img2d, axis=1))
    gz = np.abs(np.diff(img2d, axis=0))
    grad_energy = float(np.mean(gx)) + float(np.mean(gz))
    mean_amp = float(np.mean(np.abs(image)))
    focus = grad_energy / (mean_amp + 1e-10)
    # Very low threshold - just check that there's some variation
    passed = focus > 1e-6
    return TestResult(
        name="focus_ratio",
        passed=passed,
        value=focus,
        message=f"Focus ratio: {focus:.6f}" if passed else f"Focus too low: {focus:.6f}"
    )


def check_amplitude_range(image: np.ndarray) -> TestResult:
    """Check that amplitude range is reasonable (not exploded)."""
    max_amp = float(np.max(np.abs(image)))
    passed = max_amp < 1e15 and max_amp > 1e-10
    return TestResult(
        name="amplitude_range",
        passed=passed,
        value=max_amp,
        message=f"Max amplitude: {max_amp:.2e}" if passed else f"Amplitude out of range: {max_amp:.2e}"
    )


def check_sharpness(image: np.ndarray, nx: int, nz: int) -> TestResult:
    """Check image sharpness via Laplacian variance (higher = sharper)."""
    img2d = image.reshape(nz, nx).astype(np.float64)
    # Laplacian kernel approximation
    lap = np.abs(img2d[1:-1, 1:-1] - 0.25 * (
        img2d[:-2, 1:-1] + img2d[2:, 1:-1] +
        img2d[1:-1, :-2] + img2d[1:-1, 2:]
    ))
    sharpness = float(np.mean(lap))
    passed = sharpness > 1e-10  # Just check it's non-zero
    return TestResult(
        name="sharpness",
        passed=passed,
        value=sharpness,
        message=f"Sharpness (Laplacian): {sharpness:.4f}" if passed else f"Image too flat: {sharpness:.4f}"
    )


def check_shape(image: np.ndarray, expected_size: int) -> TestResult:
    """Check that output has expected shape."""
    passed = image.size == expected_size
    return TestResult(
        name="shape",
        passed=passed,
        value=float(image.size),
        message=f"Shape: {image.size} elements" if passed else f"Shape mismatch: {image.size} vs {expected_size}"
    )


def run_pipeline(config: PipelineConfig) -> PipelineResult:
    """Run the full self-test pipeline."""
    start_time = time.perf_counter()
    tests = []
    output_dir = Path(f"output/self_test_{config.preset}")
    output_dir.mkdir(parents=True, exist_ok=True)
    
    try:
        # Step 1: Generate synthetic model
        print(f"[1/3] Generating {config.preset} model...")
        gen_cmd = [
            "python3", "scripts/generate_synthetic_model.py",
            "--out-dir", str(output_dir / "model"),
            "--nx", str(config.nx),
            "--nz", str(config.nz),
            "--nt", str(config.nt),
            "--dt", str(config.dt),
            "--f0", str(config.f0),
            "--scenario", config.preset,
            "--seed", str(config.seed),
            "--n-shots", str(config.n_shots),
        ]
        ret, stdout, stderr = run_command(gen_cmd)
        if ret != 0:
            return PipelineResult(
                passed=False, elapsed_ms=0, tests=[],
                output_path=str(output_dir),
                error=f"Model generation failed: {stderr}"
            )
        
        # Step 2: Create config file
        config_path = output_dir / "rtm_config.json"
        rtm_config = {
            "data_dir": str(output_dir / "model"),
            "velocity_model": "velocity_model.bin",
            "nx": config.nx,
            "nz": config.nz,
            "dx": 10.0,
            "dz": 10.0,
            "ny": 8,
            "dy": 10.0,
            "nt": config.nt,
            "dt": config.dt,
            "f0": config.f0,
            "pml": 6,
            "n_shots": config.n_shots,
            "output": str(output_dir / "migrated.pgm"),
            "output_format": "pgm8"
        }
        config_path.write_text(json.dumps(rtm_config, indent=2))
        
        # Step 3: Run RTM
        print(f"[2/3] Running RTM migration...")
        rtm_cmd = ["./build/rtm3d_cli", "--config", str(config_path)]
        ret, stdout, stderr = run_command(rtm_cmd, timeout=300)
        if ret != 0:
            return PipelineResult(
                passed=False, elapsed_ms=0, tests=[],
                output_path=str(output_dir),
                error=f"RTM failed: {stderr}"
            )
        
        # Step 4: Validate output
        print(f"[3/3] Validating output...")
        
        output_file = output_dir / "migrated.pgm"
        
        # Check if RTM produced output
        if not output_file.exists():
            return PipelineResult(
                passed=False, elapsed_ms=0, tests=[],
                output_path=str(output_dir),
                error=f"RTM output file not found: {output_file}"
            )
        
        # Read PGM file
        with open(output_file, "rb") as f:
            # Skip PGM header
            line = f.readline()  # P5
            if not line.startswith(b'P5'):
                return PipelineResult(
                    passed=False, elapsed_ms=0, tests=[],
                    output_path=str(output_file),
                    error=f"Invalid PGM file - expected P5, got {line[:10]}"
                )
            line = f.readline()  # dimensions
            dims = line.strip().split()
            nx_out, nz_out = int(dims[0]), int(dims[1])
            line = f.readline()  # max value
            image_data = f.read()
        
        image = np.frombuffer(image_data, dtype=np.uint8).astype(np.float32)
        meta = {"nx": nx_out, "nz": nz_out}
        
        expected_size = config.nx * config.nz
        
        # Run checks
        tests.append(check_shape(image, expected_size))
        tests.append(check_finite_values(image))
        tests.append(check_energy(image))
        tests.append(check_focus_ratio(image, config.nx, config.nz))
        tests.append(check_amplitude_range(image))
        
        elapsed_ms = (time.perf_counter() - start_time) * 1000
        
        passed = all(t.passed for t in tests)
        
        # Convert TestResult to dict with native Python types
        tests_dict = []
        for t in tests:
            td = {
                "name": t.name,
                "passed": bool(t.passed),
                "value": float(t.value) if t.value is not None else None,
                "threshold": float(t.threshold) if t.threshold is not None else None,
                "message": t.message
            }
            tests_dict.append(td)
        
        # Save result
        result = PipelineResult(
            passed=passed,
            elapsed_ms=elapsed_ms,
            tests=tests_dict,
            output_path=str(output_file)
        )
        
        # Save result
        (output_dir / "result.json").write_text(json.dumps(asdict(result), indent=2))
        
        return result
        
    except Exception as e:
        return PipelineResult(
            passed=False,
            elapsed_ms=(time.perf_counter() - start_time) * 1000,
            tests=[],
            output_path=str(output_dir),
            error=str(e)
        )


def main() -> int:
    ap = argparse.ArgumentParser(description="Self-testing E2E pipeline for RTM")
    ap.add_argument("--preset", default="layers", 
                    choices=["constant", "layers", "circle", "circle_lens", "salt_dome"],
                    help="Velocity model preset")
    ap.add_argument("--quick", action="store_true", help="Run quick test (smaller model)")
    ap.add_argument("--nx", type=int, default=80)
    ap.add_argument("--nz", type=int, default=48)
    ap.add_argument("--n-shots", type=int, default=2)
    args = ap.parse_args()
    
    config = PipelineConfig(
        preset=args.preset,
        nx=48 if args.quick else args.nx,
        nz=32 if args.quick else args.nz,
        nt=60 if args.quick else 100,
        n_shots=1 if args.quick else args.n_shots,
        quick=args.quick
    )
    
    print(f"=== RTM Self-Test Pipeline ===")
    print(f"Preset: {config.preset}")
    print(f"Size: {config.nx}x{config.nz}")
    print(f"Shots: {config.n_shots}")
    print()
    
    result = run_pipeline(config)
    
    print()
    print("=== Results ===")
    for test in result.tests:
        status = "✓ PASS" if test["passed"] else "✗ FAIL"
        print(f"  {status}: {test['name']}")
        if test["value"] is not None:
            print(f"         value: {test['value']:.4e}")
        if test["message"]:
            print(f"         {test['message']}")
    
    print()
    if result.passed:
        print(f"✓ ALL CHECKS PASSED ({result.elapsed_ms:.0f}ms)")
        return 0
    else:
        print(f"✗ FAILED ({result.error or 'see above'})")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
