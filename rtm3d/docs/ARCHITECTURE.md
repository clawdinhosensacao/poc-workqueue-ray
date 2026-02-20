# Architecture

- `MarmousiLoader`: parses Marmousi JS-style arrays (`x_`, `z_`, `vel_`) and applies decimation/crop.
- `RtmEngine`: isotropic 3D acoustic finite-difference propagation + RTM imaging condition.
- `ImageIO`: normalized grayscale output for inline slice.
- `rtm3d_cli`: orchestration (load -> migrate one shot -> save image).

## Scalability hooks
- Separate physics engine module (`RtmEngine`) for future TTI kernels.
- `Volume3D` abstraction can back CPU/GPU memory variants later.
- CLI currently single-shot but can evolve into job batching.
