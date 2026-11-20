# Cache comparison results

[Usage](../README.md) | [Short descriptions](cache_summary.md) | [Implementation details](cache_details.md)

## How these results were obtained

These results were run again from GitHub commit `8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6` during the documentation review on 20 November 2026.

```bash
python3 scripts/build.py
python3 scripts/test_cache_runner.py
python3 scripts/compare_caches.py --csv results/cache_comparison.csv
```

The runner built with g++ and all eight Python integration tests passed. The comparison completed 24 runs across 22 named scenarios and produced 165 policy rows. CMake and GoogleTest were not run in this environment because CMake was not installed. The current CMake source list was checked and the previously removed comparison file is no longer listed.

Each run starts empty. Cold misses are included. Objects have equal size, values equal keys, and requests are reads only. Capacity C is measured in objects. The five online caches each receive C slots. L1+L2 receives C/3 slots for LRU (rounded down) and the rest for LFU; the two-level run is skipped when L1 would be empty.

IDEAL is calculated by Python's `ideal_hits`. It knows the whole sequence. Its result is a reference, not an online policy that can be used in a config.

Inputs are stored in [requests/comparison](../requests/comparison/), with names and descriptions in [scenarios.json](../requests/comparison/scenarios.json). The random sequences are fixed files, so rerunning the comparison does not regenerate them.

## Full table

All numbers below are **misses**. Lower is better. For L1+L2, a miss means an access to memory after both levels missed. A dash means the two-level run was skipped.

| Scenario | C | Requests | LRU | LFU | 2Q | ARC | LIRS | IDEAL | L1+L2 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| EmptyInput | 6 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ZeroCapacity | 0 | 6 | 6 | 6 | 6 | 6 | 6 | 6 | - |
| CapacityOne | 1 | 9 | 5 | 5 | 5 | 5 | 5 | 5 | - |
| CapacityTwo | 2 | 9 | 9 | 7 | 8 | 9 | 7 | 6 | - |
| SmallestMultilevel | 3 | 10 | 4 | 4 | 5 | 4 | 4 | 4 | 5 |
| CapacityNotDivisibleByThree | 5 | 14 | 10 | 8 | 11 | 8 | 8 | 7 | 9 |
| HotKeysAndScans | 6 | 30 | 18 | 14 | 16 | 14 | 14 | 14 | 14 |
| WorkingSetFits | 9 | 18 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| ChangingWorkingSet | 6 | 24 | 6 | 6 | 6 | 6 | 6 | 6 | 12 |
| UniqueKeys | 12 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 16 |
| RepeatedData | 6 | 90 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |
| FrequentlyAccessedItem | 6 | 102 | 91 | 81 | 82 | 81 | 81 | 81 | 81 |
| CatalogScanAndFavorites | 9 | 146 | 95 | 83 | 95 | 83 | 83 | 83 | 83 |
| SequentialReading | 12 | 120 | 120 | 120 | 120 | 120 | 120 | 120 | 120 |
| CyclicWorkingSetExceedsCache | 6 | 140 | 140 | 45 | 69 | 140 | 45 | 29 | 83 |
| SwitchingInterests | 6 | 300 | 12 | 186 | 12 | 17 | 18 | 12 | 243 |
| ReturningToPreviousProject | 6 | 180 | 12 | 50 | 12 | 15 | 17 | 10 | 65 |
| ShortBurstsOfNewData | 6 | 200 | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| MovingWorkingWindow | 6 | 240 | 23 | 189 | 23 | 23 | 23 | 23 | 211 |
| PopularitySkewWithLongTail | 6 | 1000 | 547 | 450 | 496 | 522 | 470 | 340 | 740 |
| PopularitySkewWithLongTail | 12 | 1000 | 326 | 255 | 304 | 314 | 281 | 175 | 399 |
| PopularitySkewWithLongTail | 24 | 1000 | 98 | 101 | 98 | 104 | 101 | 55 | 189 |
| UniformRandomRequests | 12 | 1000 | 637 | 625 | 629 | 643 | 643 | 344 | 677 |
| InterleavedUserSessions | 9 | 300 | 63 | 63 | 66 | 63 | 63 | 63 | 63 |

## What the scenarios show

### Repeated data

In `RepeatedData`, three keys repeat 30 times. All policies have only three misses: the first load of each key. The working set fits, so replacement choices do not matter much.

In `ShortBurstsOfNewData`, each of 40 objects is requested five times in a row. Every policy has 40 misses and 160 hits. L1 serves all 160 hits in the two-level cache.

The same total hit count can hide a different use of levels. In `RepeatedData`, L1 has only two slots for the cycle of three keys, so all 87 hierarchy hits happen in L2.

### Popular objects between scans

`FrequentlyAccessedItem` first repeats key 1, then returns to it after groups of eight new keys. With capacity 6, LRU removes key 1 during each group and has 91 misses. LFU, ARC and LIRS keep it and have 81. The hierarchy also has 81; 2Q has 82.

`CatalogScanAndFavorites` returns to three favourite objects after long scans. LRU and this 2Q configuration have 95 misses, while LFU, ARC, LIRS and the hierarchy have 83. This is a result for these queue sizes and this order of requests, not a general ranking of 2Q.

### No repeated requests

`SequentialReading` has 120 different keys. Every policy, including IDEAL, has 120 misses. There are no future returns that would make stored data useful. These equal results are expected control cases.

### A cycle just larger than the cache

`CyclicWorkingSetExceedsCache` repeats seven keys with capacity 6. LRU removes the key needed next, so every request misses: 140 misses. ARC also has 140 on this sequence. LFU and LIRS have 45, 2Q has 69, and IDEAL has 29.

In this LFU implementation, equal-frequency entries follow the list's insertion and movement order. Some keys survive long enough to collect hits. The result depends on that tie handling, even though all seven keys have the same total request count.

### A change of interests

`SwitchingInterests` first uses keys 1..6, then switches to 11..16 and never returns. LRU and 2Q have 12 misses, matching IDEAL. ARC has 17 and LIRS has 18.

LFU has 186 misses. Old keys keep high counts, while new keys start at 1. Since counts do not age, the cache can keep data that is no longer useful.

The hierarchy has 243 misses here. L1 cannot hold the six-key cycle, and the smaller LFU level also keeps old entries. More levels do not automatically mean fewer misses.

### Returning to an earlier project

`ReturningToPreviousProject` uses four keys from project A, four from project B, then A again. LRU and 2Q have 12 misses, ARC 15, LIRS 17, LFU 50 and the hierarchy 65. IDEAL has 10.

Keeping some old data can help on the return, but it also takes space from the active project. This sequence shows that trade-off.

### A moving window

`MovingWorkingWindow` repeats a window of four keys, then moves it by one key. LRU, 2Q, ARC, LIRS and IDEAL have 23 misses, one per distinct object. LFU has 189 and the hierarchy has 211.

Recently used keys remain useful, while old popular keys leave the window. Here recency is a better guide than a count that never decreases.

### Uneven popularity

`PopularitySkewWithLongTail` uses 32 keys with integer weights close to 1/rank. At capacity 6, LFU has 450 misses versus LRU's 547. At capacity 12, the counts are 255 and 326. Frequency helps keep popular objects.

At capacity 24, LRU is slightly better: 98 misses versus 101. More of the working set fits, and individual choices about rare keys matter. The best result can change with capacity even on the same sequence.

### Uniform random requests

`UniformRandomRequests` has 1000 requests among 32 keys at capacity 12. The five online policies have between 625 and 643 misses. IDEAL has 344 because it knows future requests.

One fixed random sequence is not enough to prove that one online policy is better for all uniform workloads. Such a claim would need several independent sequences and a measure of variation.

### Mixed sessions

`InterleavedUserSessions` mixes repeated visits to three common pages with pairs of requests to new pages. Most policies have 63 misses; 2Q has 66. The hierarchy serves 60 hits in L1 and 177 in L2.

These session and browsing examples are ways to interpret synthetic traces. They are not recorded actions of real users.

## How to read the hierarchy results

The total slot budget is the same as for a single-level cache, but levels can hold duplicate keys. The number of distinct stored objects can therefore be smaller than C.

Also, L2 only sees L1 misses. Its frequency counts do not measure popularity in the full request stream. This explains why LRU+LFU is not equivalent to one LFU cache of size C.

The tables do not measure access latency. A hierarchy can serve some requests in a fast upper level, but this project does not assign timing costs to levels or memory. Hit counts alone cannot establish a runtime speedup.

## Reproduce a small result

```bash
python3 scripts/compare_caches.py --scenario HotKeysAndScans
```

For C=6 and 30 requests:

| Cache | Hits | Misses |
|---|---:|---:|
| LRU | 12 | 18 |
| LFU | 16 | 14 |
| 2Q | 14 | 16 |
| ARC | 16 | 14 |
| LIRS | 16 | 14 |
| IDEAL | 16 | 14 |
| L1+L2 | 16 | 14 |

All stored sequences and capacities are needed to reproduce the table. Changing a policy, its queue sizes, tie handling or request order can change the result.
