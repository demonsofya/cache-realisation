# Cache implementation

A C++ simulator for comparing LRU, LFU, 2Q, ARC and LIRS caches, including
multi-level hierarchies. IDEAL provides an offline reference for comparisons.

## Build and test

Tested on Linux with GCC 14.2.0, CMake 4.4.5 and Python 3.12.14.
CMake uses installed CLI11 and GoogleTest packages or downloads pinned versions
when they are missing. Python is only used for experiments and integration tests.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Use `-DBUILD_TESTING=OFF` to build without tests.

## Run

Create `cache.cfg` with the number of levels and their policies:

```text
2 LRU LFU
```

Create `requests.txt` with total capacity, request count and keys:

```text
3 6
1 2 3 1 2 3
```

```bash
./build/cache_runner cache.cfg requests.txt
```

The output is the number of hits. The hierarchy splits capacity using weights `1, 2, 4, ...`.
Levels replace entries independently and may contain duplicate keys.
Use `--help` for other distributions, exact sizes, JSON output.
See [hierarchy settings](docs/hierarchy.md) for examples and design details.

## Compare workloads

```bash
python3 scripts/compare_caches.py --csv results/comparison.csv
```

- `compare_caches.py`: compare saved scenarios and level configurations.
- `generate_requests.py`: create request sequences.
- `run_benchmarks.py`: run config files against request files.
- `test_cache_runner.py`: check command-line input and output; also run by CTest.

Each script has `--help`. C++ unit tests check cache policies and allocation.

[Cache descriptions](docs/cache_summary.md) · [Implementation details](docs/cache_details.md) · [Results](docs/results.md)
