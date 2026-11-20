# Cache Realisation

A C++23 project for comparing cache replacement algorithms. It supports LRU, LFU, 2Q, ARC and LIRS, plus an ideal cache as a reference. A configuration file selects the algorithms and the number of cache levels. Python scripts generate requests, run comparisons and save results as CSV.

The model uses equal-size objects. A key is a nonnegative integer, and its value is the key itself. Cache size means the number of objects, not bytes. The program counts hits and misses; it does not measure hardware access time or model writes, TTL or prefetching.

## Documentation

- [Short cache descriptions](docs/cache_summary.md)
- [Detailed descriptions with code links](docs/cache_details.md)
- [Measured results and their meaning](docs/results.md)

## Build and use

### Requirements

- Python 3 and a C++ compiler with C++23 support, such as a recent g++.
- No extra Python packages are needed.
- The CMake build also requires CMake 3.20 or newer and GoogleTest 1.14 or newer.

Commands below run from the project root on Linux or WSL.

```bash
git clone https://github.com/demonsofya/cache-realisation.git
cd cache-realisation
python3 scripts/build.py
```

The script compiles `src/cache_runner.cpp` and `src/cache_hierarchy.cpp` and creates `build/cache_runner`. It does not need CMake or GoogleTest.

To select the compiler or output path:

```bash
python3 scripts/build.py --cxx g++ --output build/cache_runner
```

### First run

```bash
./build/cache_runner configs/lru.cfg requests/slide_repeated.txt
```

The config is `1 LRU`. The input is `2 4 1 2 1 2`. The first two requests miss, and the next two hit. The exact output is:

```text
2
```

The normal mode prints only the total number of hits and a newline.

### Configuration files

A `.cfg` file is a plain text file:

```text
number_of_levels policy_1 policy_2 ... policy_N
```

For example:

```text
2 LRU LFU
```

This selects LRU for L1 and LFU for L2. Available names are `LRU`, `LFU`, `2Q`, `ARC` and `LIRS`. Names are case-sensitive. The number of names must match the level count. IDEAL is an offline reference and cannot be a level in this config.

| File | Levels |
|---|---|
| `configs/lru.cfg` | LRU |
| `configs/lfu.cfg` | LFU |
| `configs/two_levels.cfg` | LRU, LFU |
| `configs/three_levels.cfg` | LRU, LFU, ARC |
| `configs/lfu_arc.cfg` | LFU, ARC |

### Request files

A request file contains one run:

```text
cache_capacity request_count key_1 key_2 ... key_M
```

For example:

```text
6 8
1 2 3 1 2 4 1 2
```

The capacity is 6 objects and there are exactly 8 requests. Spaces and newlines both separate tokens. The runner rejects negative numbers, invalid keys, missing requests and extra requests. Comments start with a separate token beginning with `#` and continue to the end of the line.

### Level sizes

The input capacity is the **total** size. For N levels, the weights are `1:2:...:N`. All levels except the last receive their rounded-down share; the last receives the remainder.

| Total capacity | Levels | Level sizes |
|---:|---:|---|
| 6 | 1 | 6 |
| 6 | 2 | 2, 4 |
| 6 | 3 | 1, 2, 3 |
| 10 | 3 | 1, 3, 6 |
| 2 | 2 | 0, 2 |

A level with zero capacity stores nothing and passes requests on. The runner allows this. The prepared comparison script skips its two-level comparison when L1 would have zero capacity.

A request visits L1 first, then lower levels until a hit. A miss at every level counts as one memory access. Levels replace objects independently and may store copies of the same key.

### Output and stdin

```bash
./build/cache_runner configs/two_levels.cfg requests/hot_keys.txt
```

This example prints `16`. To see per-level statistics:

```bash
./build/cache_runner configs/two_levels.cfg requests/hot_keys.txt --json
```

Here L1 has 12 hits, L2 has 4 hits and the system has 14 memory accesses. Do not add per-level misses to get total misses: a request can miss at several levels.

To read requests from stdin or save the answer:

```bash
./build/cache_runner configs/lru.cfg < requests/slide_repeated.txt
./build/cache_runner configs/lru.cfg requests/slide_repeated.txt > answer.txt
```

When typing input by hand, finish it with Ctrl+D on Linux. The runner checks for extra tokens after the declared request count, so it needs the end of input. Errors go to stderr and return exit code 1.

## Python scripts

| Script | Purpose |
|---|---|
| `build.py` | Compile the runner. Options: `--cxx`, `--output`. |
| `generate_requests.py` | Write eight workloads. Options: `--capacity`, `--seed`, `--output-dir`. |
| `compare_caches.py` | Compare all policies and IDEAL on the saved scenarios. Options: `--binary`, `--scenario`, `--list`, `--csv`, `--delimiter`. |
| `run_benchmarks.py` | Run every chosen config against every chosen request file. Options: `--binary`, `--configs`, `--requests`, `--csv`, `--delimiter`. |
| `test_cache_runner.py` | Run eight integration tests. Set `CACHE_RUNNER` to select another binary. |

### Compare the saved scenarios

```bash
python3 scripts/compare_caches.py --list
python3 scripts/compare_caches.py --scenario HotKeysAndScans
python3 scripts/compare_caches.py --csv results/cache_comparison.csv
```

There are 22 named scenarios and 24 runs. The popularity scenario uses three capacities. Input files and descriptions are in `requests/comparison/`. Each C++ run starts with empty caches. IDEAL is calculated in Python. The two-level row uses LRU + LFU with a 1/3 and 2/3 split.

The script checks that hits do not exceed IDEAL and that counters agree. It prints tables without GoogleTest messages.

### Generate and compare your own workloads

```bash
python3 scripts/generate_requests.py --capacity 6 --seed 2026
python3 scripts/run_benchmarks.py \
    --configs configs/*.cfg \
    --requests requests/generated/*.txt \
    --csv results/comparison.csv
```

This runs five configs on eight workloads: 40 runs. Each run starts a new process. `*.cfg` and `*.txt` are expanded by the shell. The scripts stop if a C++ run fails.

To compare a larger capacity with the same generated requests:

```bash
python3 scripts/generate_requests.py --capacity 12 --seed 2026 --output-dir requests/capacity12
python3 scripts/run_benchmarks.py --configs configs/*.cfg --requests requests/capacity12/*.txt
```

Generation overwrites files with the same names in the output directory. It does not change `requests/comparison/`. Python-generated random workloads differ from the saved C++-generated comparison sequences, even with the same seed.

### CSV files

Both comparison scripts use `;` by default and UTF-8 with BOM. For comma-separated output, add `--delimiter ','`. If Excel opens a file in one column, import it through **Data > From Text/CSV** and select the matching separator.

Keep `.py` source files in UTF-8 too. CSV encoding and Python source encoding are separate settings.

## Tests and other build options

The main checked workflow does not need GoogleTest:

```bash
python3 scripts/test_cache_runner.py
```

For a different runner path:

```bash
CACHE_RUNNER=build/cache_runner python3 scripts/test_cache_runner.py -v
```

### Build with CMake and run GoogleTest

The current CMake file requires GoogleTest 1.14 or newer even when you only want to build the runner. It does not use `BUILD_TESTING`. For a build without GoogleTest, use `scripts/build.py` above.

With CMake 3.20 or newer and GoogleTest installed:

```bash
cmake -S . -B build-cmake
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
python3 scripts/compare_caches.py --binary build-cmake/cache_runner
```

Use `-DGTest_DIR=/path/to/lib/cmake/GTest` when CMake needs an explicit GoogleTest config location. The source list was reviewed at commit `8d147bb`. CMake and GoogleTest execution were not checked in the review environment because CMake was not installed. The Python build, runner tests and comparison commands were checked.

### Older demonstration program

`src/cache.cpp` is a separate demo that prints named results for all policies. It also uses the C++ IDEAL implementation. It does not accept a config file. To build it directly:

```bash
g++ -std=c++23 -O2 -I include src/cache.cpp src/ideal_cache.cpp -o build/cache
./build/cache < requests/slide_repeated.txt
```

Use `cache_runner` when the required output is only a hit count.

## Results

On `HotKeysAndScans`, with 30 requests and total capacity 6, LRU has 18 misses, LFU has 14, and LRU + LFU has 14. On `SwitchingInterests`, LRU has 12 misses while LFU has 186. The best choice depends on the request pattern.

See [the full results](docs/results.md) for all 24 runs, commands and explanations. These are hit-count experiments on synthetic data, not timing benchmarks or measurements of real users.

## Layout

| Path | Contents |
|---|---|
| `include/` | Cache classes and interfaces |
| `src/` | Runner, hierarchy, IDEAL implementation and demo |
| `test/` | GoogleTest correctness tests |
| `scripts/` | Build, generation, comparison and integration tests |
| `configs/` | Example level configurations |
| `requests/` | Example and comparison inputs |
| `docs/` | Cache descriptions and measured results |

## Similar projects

The documentation layout was informed by [libCacheSim](https://github.com/1a1a11a/libCacheSim) and [cache-simulator](https://github.com/TheMuffinMan1320/cache-simulator): a short overview, build steps, a first example, input format, comparison commands and results. Their simulator features and measurements are separate from this project.
