# Level sizes

The config format is unchanged: a level count followed by policy names, for
example `3 LRU ARC LIRS`. Request input starts with total capacity and request
count, followed by keys.

## Distributions

| Option | Weights | Sizes for total capacity 21 |
|---|---|---|
| `--distribution equal` | 1, 1, 1 | 7, 7, 7 |
| `--distribution linear` | 1, 2, 3 | 3, 7, 11 |
| `--distribution geometric` (default) | 1, 2, 4 | 3, 6, 12 |
| `--distribution geometric --ratio 4` | 1, 4, 16 | 1, 4, 16 |
| `--sizes 1,5,15` | Explicit sizes | 1, 5, 15 |

Weighted sizes are rounded down; the last level receives the remainder so no
slots are lost. The integer geometric ratio must be at least 2. Explicit sizes
must sum to the input capacity and cannot be combined with distribution options.
A multi-level allocation with an empty level is rejected. Single-level zero
capacity remains supported for boundary tests. Arithmetic overflow is rejected.

```bash
./build/cache_runner configs/three_levels.cfg requests/comparison/PopularitySkewWithLongTail_24.txt --distribution equal --json
./build/cache_runner configs/three_levels.cfg requests/comparison/PopularitySkewWithLongTail_24.txt --ratio 4 --json
./build/cache_runner configs/three_levels.cfg requests/comparison/PopularitySkewWithLongTail_24.txt --sizes 2,6,16 --json
```

Levels still operate independently and can contain duplicate resident keys.
Exclusive placement is not part of this update.

## Why configurable growth

[AMD's Ryzen 9 9950X specifications](https://www.amd.com/en/products/processors/desktops/ryzen/9000-series/amd-ryzen-9-9950x.html)
list total L1/L2/L3 capacities of 1280 KB, 16 MB and 64 MB. This is approximately
1:12.8:51.2, rather than 1:2:3. These are whole-processor totals, not a single
core's access path. The simulator does not model CPU topology or access latency.
A configurable growth factor and explicit sizes allow experiments without
assuming that one ratio fits all hardware. Factor 2 is a convenient default.

Equal and geometric allocation were inspired by
[nhwc's capacity policies](https://github.com/lon3ry/nhwc/blob/c7a9a58adfc06b38f41b4ecaf6a477545634df0d/bench/source/capacity_policy.py).
This implementation also preserves rounding remainders.

## Experiments

```bash
python3 scripts/compare_caches.py --all-pairs --distribution equal --csv results/equal.csv
python3 scripts/compare_caches.py --all-pairs --ratio 4 --csv results/geometric.csv
python3 scripts/compare_caches.py --levels LRU ARC LIRS --scenario PopularitySkewWithLongTail
```

`--all-pairs` compares all 25 ordered pairs. Allocations that would create an empty
level are explicitly skipped. CSV output records sizes and distribution settings.
`run_benchmarks.py` also accepts `--distribution`, `--ratio` and `--sizes`.
Python runs the built executable; it does not build it.

## Dependencies

CMake finds installed CLI11 and GoogleTest or downloads pinned revisions.
GoogleTest is only needed with `BUILD_TESTING=ON`. To use local installations:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/dependencies -DCACHE_FETCH_DEPENDENCIES=OFF
```

CTest runs cache and allocation unit tests, plus CLI integration tests when Python
is available. CLI11 provides argument validation and `--help`.
