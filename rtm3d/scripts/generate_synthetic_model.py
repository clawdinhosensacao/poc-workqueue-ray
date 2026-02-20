#!/usr/bin/env python3
import json
import math
import os

out_dir = os.environ.get("RTM3D_SYNTH_OUT", "data/synthetic")
os.makedirs(out_dir, exist_ok=True)

nx, nz = 160, 90
dx, dz = 12.5, 12.5

x = [i * dx for i in range(nx)]
z = [i * dz for i in range(nz)]
vel = []
for iz in range(nz):
    row = []
    for ix in range(nx):
        base = 1500 + 0.7 * z[iz]
        anomaly = 350.0 * math.exp(-(((ix - nx * 0.55) / 22.0) ** 2 + ((iz - nz * 0.45) / 12.0) ** 2))
        row.append(base + anomaly)
    vel.append(row)

with open(os.path.join(out_dir, "x.json"), "w") as f:
    json.dump(x, f)
with open(os.path.join(out_dir, "z.json"), "w") as f:
    json.dump(z, f)
with open(os.path.join(out_dir, "vel.json"), "w") as f:
    json.dump(vel, f)

print(f"Synthetic model written to {out_dir}")
