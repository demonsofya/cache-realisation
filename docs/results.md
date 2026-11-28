# Cache comparison results

Measured after the exclusive hierarchy update on 28 November 2026, using the
fixed traces in [requests/comparison](../requests/comparison/). Each cache starts
empty. All numbers are **misses**; lower is better. Single policies receive the
whole capacity C. The hierarchy uses exclusive LRU+LFU with sizes `floor(C/3)`
and the remainder. A dash means an allocation with an empty level was skipped.
IDEAL is calculated by the Python Belady reference.

| Scenario | C | Requests | LRU | LFU | 2Q | ARC | LIRS | IDEAL | Exclusive LRU+LFU |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| EmptyInput | 6 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ZeroCapacity | 0 | 6 | 6 | 6 | 6 | 6 | 6 | 6 | — |
| CapacityOne | 1 | 9 | 5 | 5 | 5 | 5 | 5 | 5 | — |
| CapacityTwo | 2 | 9 | 9 | 7 | 8 | 9 | 7 | 6 | — |
| SmallestMultilevel | 3 | 10 | 4 | 4 | 5 | 4 | 4 | 4 | 5 |
| CapacityNotDivisibleByThree | 5 | 14 | 10 | 8 | 11 | 8 | 8 | 7 | 7 |
| HotKeysAndScans | 6 | 30 | 18 | 14 | 16 | 14 | 14 | 14 | 16 |
| WorkingSetFits | 9 | 18 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| ChangingWorkingSet | 6 | 24 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| UniqueKeys | 12 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 16 |
| RepeatedData | 6 | 90 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |
| FrequentlyAccessedItem | 6 | 102 | 91 | 81 | 82 | 81 | 81 | 81 | 90 |
| CatalogScanAndFavorites | 9 | 146 | 95 | 83 | 95 | 83 | 83 | 83 | 92 |
| SequentialReading | 12 | 120 | 120 | 120 | 120 | 120 | 120 | 120 | 120 |
| CyclicWorkingSetExceedsCache | 6 | 140 | 140 | 45 | 69 | 140 | 45 | 29 | 40 |
| SwitchingInterests | 6 | 300 | 12 | 186 | 12 | 17 | 18 | 12 | 186 |
| ReturningToPreviousProject | 6 | 180 | 12 | 50 | 12 | 15 | 17 | 10 | 121 |
| ShortBurstsOfNewData | 6 | 200 | 40 | 40 | 40 | 40 | 40 | 40 | 40 |
| MovingWorkingWindow | 6 | 240 | 23 | 189 | 23 | 23 | 23 | 23 | 159 |
| PopularitySkewWithLongTail | 6 | 1000 | 547 | 450 | 496 | 522 | 470 | 340 | 651 |
| PopularitySkewWithLongTail | 12 | 1000 | 326 | 255 | 304 | 314 | 281 | 175 | 442 |
| PopularitySkewWithLongTail | 24 | 1000 | 98 | 101 | 98 | 104 | 101 | 55 | 142 |
| UniformRandomRequests | 12 | 1000 | 637 | 625 | 629 | 643 | 643 | 344 | 621 |
| InterleavedUserSessions | 9 | 300 | 63 | 63 | 66 | 63 | 63 | 63 | 63 |

The hierarchy now stores each resident key at exactly one level. A lower-level
hit promotes the object to L1; victims move down. Ghost history is only metadata.
This makes all slots available to distinct resident keys, but changes the stream
seen by each policy. In particular, lower LFU levels admit victims from above and
lose hits to promotion. Exclusive placement need not improve hits on every trace.

For `HotKeysAndScans`, the old independent LRU+LFU hierarchy had 14 misses;
the exclusive hierarchy has 16. For the cycle just larger than the cache, the old
hierarchy had 83 misses and the exclusive version has 40. These examples show why
placement must be evaluated with the workload rather than ranked by slot use alone.

Repeated small sets test initial loading; unique scans produce no hits even for
IDEAL. Changing interests and moving windows expose LFU's counts without ageing.
These synthetic traces measure hit counts, not access latency or throughput.

## Reproduce

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 scripts/compare_caches.py --csv results/exclusive.csv
python3 scripts/compare_caches.py --all-pairs --csv results/exclusive_pairs.csv
```

All 54 CTest checks passed, including the 125 ordered triples of policies,
transfer checks for each policy, and CLI integration tests. The comparison also
checks totals and the IDEAL bound. Passing tests supports the tested cases; it is
not a proof for every possible sequence or key type.
