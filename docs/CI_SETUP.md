# CI Workflow Setup

The following GitHub Actions workflow is recommended for automated testing.

Create file `.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y g++ python3 python3-pip python3-numpy

    - name: Build
      run: make

    - name: Run tests
      run: make test

    - name: Run self-test pipeline
      run: |
        python3 scripts/self_test_pipeline.py --quick --preset layers
        python3 scripts/self_test_pipeline.py --quick --preset circle

  benchmark:
    runs-on: ubuntu-latest
    needs: build-and-test

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y g++ python3 python3-pip python3-numpy

    - name: Build
      run: make

    - name: Run all presets
      run: |
        for preset in constant layers circle circle_lens salt_dome; do
          echo "=== Testing $preset ==="
          python3 scripts/self_test_pipeline.py --quick --preset $preset
        done
```

**Note:** Requires a PAT with `workflow` scope to push.
