# RTM 3D CLI (Isotropic, CPU)

Modern C++ (C++20) command-line starter for **3D acoustic RTM** focused on:
- clean architecture (`loader`, `engine`, `io`)
- maintainability and extensibility (future TTI / GPU)
- unit + e2e style tests

## Data source (Marmousi)
Downloaded from:
- https://github.com/mlamoureux/Marmousi (`x.json`, `z.json`, `vel.json`)

## Build
Preferred (if CMake is installed):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Portable fallback:
```bash
make -j
```

## Run migration (single shot)
```bash
mkdir -p output
./build/rtm3d_cli data output/migrated_inline.pgm
```

## Tests
```bash
# CMake route:
ctest --test-dir build --output-on-failure

# Make route:
make test
```

## Export inline to PNG
```bash
python3 scripts/pgm_to_png.py output/migrated_inline.pgm output/migrated_inline.png
```

## Notes
- Current version starts with isotropic CPU RTM.
- Uses Marmousi 2D velocity extruded into 3D along y.
- Imaging condition: source/receiver cross-correlation.
- For runtime sanity, model is decimated/cropped in this initial baseline.
