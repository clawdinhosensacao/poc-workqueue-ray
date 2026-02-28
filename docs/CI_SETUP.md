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

    - name: Unified checks (build + unit + parity smoke + static)
      run: |
        sudo apt-get install -y cppcheck
        make check

    - name: Run E2E synthetic benchmark
      run: |
        ./tests/e2e_synthetic.sh

```

Optional: keep heavier benchmark/preset sweeps in a separate scheduled workflow to avoid slowing down PR feedback.

**Note:** Updating `.github/workflows/*` requires a PAT with `workflow` scope to push.
