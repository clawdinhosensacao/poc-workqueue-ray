#!/usr/bin/env python3
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]


def parse_summary(path: pathlib.Path, prefix: str):
    if not path.exists():
        return None
    text = path.read_text(encoding="utf-8")
    m = re.search(rf"{prefix}_summary\s+([^\n]+)", text)
    if not m:
        return None
    fields = {}
    for tok in m.group(1).split():
        if "=" in tok:
            k, v = tok.split("=", 1)
            fields[k] = v
    return fields


wq = parse_summary(ROOT / "results" / "workqueue_manager.log", "wq")
ray = parse_summary(ROOT / "results" / "ray.log", "ray")

lines = ["benchmark_summary"]
if wq:
    lines.append(
        f"workqueue done={wq.get('done')} failed={wq.get('failed')} wall_s={wq.get('wall_s')} tasks_per_s={wq.get('tasks_per_s')}"
    )
else:
    lines.append("workqueue summary not found")

if ray:
    lines.append(
        f"ray done={ray.get('done')} failed={ray.get('failed')} wall_s={ray.get('wall_s')} tasks_per_s={ray.get('tasks_per_s')}"
    )
else:
    lines.append("ray summary not found")

if wq and ray:
    wq_wall = float(wq["wall_s"])
    ray_wall = float(ray["wall_s"])
    lines.append(f"overhead_eval wall_ratio_workqueue_over_ray={wq_wall / ray_wall:.3f}")

text = "\n".join(lines)
print(text)
(ROOT / "results" / "benchmark_summary.txt").write_text(text + "\n", encoding="utf-8")
