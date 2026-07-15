# CI Benchmark Pipeline Plan

## Goal
Automated benchmark regression detection on every PR and main branch push.

---

## Phase 1: GitHub Actions Workflow

### File: `.github/workflows/benchmark.yml`

```yaml
name: Benchmarks

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  benchmark:
    runs-on: windows-latest

    steps:
    - uses: actions/checkout@v4

    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release

    - name: Build benchmarks
      run: cmake --build build --config Release --target orderbook_bench_all

    - name: Run benchmarks
      run: |
        ./build/Release/orderbook_bench_all.exe \
          --benchmark_out=benchmark_results.json \
          --benchmark_out_format=json \
          --benchmark_repetitions=3

    - name: Upload results
      uses: actions/upload-artifact@v4
      with:
        name: benchmark-results
        path: benchmark_results.json
```

---

## Phase 2: Baseline Comparison

### Option A: Compare against stored baseline

1. Store baseline in `bench/baselines/main.json`
2. On PR, compare new results against baseline
3. Fail if any benchmark regresses > 10%

```yaml
    - name: Compare against baseline
      run: |
        python scripts/compare_benchmarks.py \
          bench/baselines/main.json \
          benchmark_results.json \
          --threshold 0.10
```

### Option B: Use github-action-benchmark

```yaml
    - name: Store benchmark result
      uses: benchmark-action/github-action-benchmark@v1
      with:
        tool: 'googlecpp'
        output-file-path: benchmark_results.json
        github-token: ${{ secrets.GITHUB_TOKEN }}
        auto-push: true
        alert-threshold: '110%'
        comment-on-alert: true
        fail-on-alert: true
```

This creates a GitHub Pages site with benchmark history graphs.

---

## Phase 3: Comparison Script

### File: `scripts/compare_benchmarks.py`

```python
#!/usr/bin/env python3
"""Compare benchmark results against baseline."""

import json
import sys
from pathlib import Path

def load_benchmarks(path: Path) -> dict[str, float]:
    with open(path) as f:
        data = json.load(f)
    return {
        b["name"]: b["real_time"]
        for b in data["benchmarks"]
    }

def compare(baseline_path: Path, current_path: Path, threshold: float) -> int:
    baseline = load_benchmarks(baseline_path)
    current = load_benchmarks(current_path)

    regressions = []
    improvements = []

    for name, current_time in current.items():
        if name not in baseline:
            continue
        baseline_time = baseline[name]
        ratio = current_time / baseline_time

        if ratio > 1 + threshold:
            regressions.append((name, baseline_time, current_time, ratio))
        elif ratio < 1 - threshold:
            improvements.append((name, baseline_time, current_time, ratio))

    if regressions:
        print("REGRESSIONS DETECTED:")
        for name, base, curr, ratio in regressions:
            print(f"  {name}: {base:.1f}ns -> {curr:.1f}ns (+{(ratio-1)*100:.1f}%)")
        return 1

    if improvements:
        print("Improvements:")
        for name, base, curr, ratio in improvements:
            print(f"  {name}: {base:.1f}ns -> {curr:.1f}ns ({(ratio-1)*100:.1f}%)")

    print("No regressions detected.")
    return 0

if __name__ == "__main__":
    baseline = Path(sys.argv[1])
    current = Path(sys.argv[2])
    threshold = float(sys.argv[3]) if len(sys.argv) > 3 else 0.10
    sys.exit(compare(baseline, current, threshold))
```

---

## Phase 4: PR Comment with Results

Add benchmark summary as PR comment:

```yaml
    - name: Comment PR with results
      if: github.event_name == 'pull_request'
      uses: actions/github-script@v7
      with:
        script: |
          const fs = require('fs');
          const results = JSON.parse(fs.readFileSync('benchmark_results.json'));

          let table = '| Benchmark | Time (ns) | CPU (ns) |\n|-----------|-----------|----------|\n';
          for (const b of results.benchmarks) {
            table += `| ${b.name} | ${b.real_time.toFixed(1)} | ${b.cpu_time.toFixed(1)} |\n`;
          }

          github.rest.issues.createComment({
            owner: context.repo.owner,
            repo: context.repo.repo,
            issue_number: context.issue.number,
            body: `## Benchmark Results\n\n${table}`
          });
```

---

## Implementation Order

1. **Create workflow file** - `.github/workflows/benchmark.yml`
2. **Generate initial baseline** - Run locally, commit `bench/baselines/main.json`
3. **Add comparison script** - `scripts/compare_benchmarks.py`
4. **Test on PR** - Open test PR, verify workflow runs
5. **Add PR commenting** - Optional polish

---

## Directory Structure After Implementation

```
.github/
└── workflows/
    └── benchmark.yml

bench/
├── baselines/
│   └── main.json           # Baseline from main branch
├── fixtures/
│   └── book_fixtures.h
├── add_order_bench.cpp
├── cancel_order_bench.cpp
├── depth_bench.cpp
└── level_info_bench.cpp

scripts/
└── compare_benchmarks.py
```

---

## Considerations

### Runner Consistency
- GitHub-hosted runners have variable performance
- For accurate comparisons, use self-hosted runner or relative comparisons only
- Alternative: Run baseline and PR benchmarks in same job, compare directly

### Noise Reduction
- Use `--benchmark_repetitions=5` minimum
- Use `--benchmark_min_time=1` for stability
- Consider `--benchmark_enable_random_interleaving=true`

### Windows-Specific
- Use `windows-latest` runner
- Paths use backslashes in some contexts
- MSVC compiler flags already configured in CMakeLists.txt
