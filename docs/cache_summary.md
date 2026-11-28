# Cache descriptions: short version

[Usage](../README.md) | [Detailed version](cache_details.md) | [Results](results.md)

## LRU: Least Recently Used

A list stores key-value pairs in access order, and an `unordered_map` stores the list iterator for each key. A hit moves the entry to the front with `splice`. A full cache removes the last entry, which was used least recently. New entries go to the front.

Code: [lru_cache.hpp](../include/lru_cache.hpp).

## LFU: Least Frequently Used

A list stores pairs of keys and access counts. An `unordered_map` stores values. The list is ordered by frequency, with the lowest count at the front. New entries start at 1. A hit raises the count and moves the entry. When full, the cache removes the front entry. Counts do not decrease over time.

Code: [lfu_cache.hpp](../include/lfu_cache.hpp).

## 2Q: Two Queues

The implementation uses `A1in`, `A1out` and `Am`. `A1in` stores new objects, `A1out` stores only keys removed from `A1in`, and `Am` is an LRU cache for objects that return from `A1out`. A hit inside `A1in` leaves the object there. A match in `A1out` is a miss because the value must be loaded again.

Code: [twoq_cache.hpp](../include/twoq_cache.hpp).

## ARC: Adaptive Replacement Cache

ARC uses four lists. `T1` stores recent objects, and `T2` stores objects used again. `B1` and `B2` keep only keys removed from those lists. The target `p_` changes the balance: a match in `B1` increases the target for `T1`; a match in `B2` decreases it. Matches in these history lists still count as misses.

Code: [arc_cache.hpp](../include/arc_cache.hpp).

## LIRS: Low Inter-reference Recency Set

LIRS separates objects into protected LIR entries and less protected HIR entries. Stack `S` tracks access history; queue `Q` stores resident HIR entries and selects objects to remove. Some removed HIR keys stay in the stack as history. A returning key can become LIR, while an older LIR entry becomes HIR. The HIR size is about one quarter of the cache, with at least one slot for a nonempty cache.

Code: [lirs_cache.hpp](../include/lirs_cache.hpp).

## IDEAL: Belady's optimal cache

IDEAL knows all future requests. On a full-cache miss, it removes the object whose next use is farthest away. An object that will never be used again is a suitable choice. The implementation first finds the next use of every request, then counts hits. It gives the minimum number of misses for the project's equal-size, initially empty, demand-loaded single-cache model.

Code: [ideal_cache.hpp](../include/ideal_cache.hpp), [ideal_cache.cpp](../src/ideal_cache.cpp). The Python comparison has its own `ideal_hits` function in [compare_caches.py](../scripts/compare_caches.py).

## Multiple cache levels

The runner combines different policies in an exclusive hierarchy. It probes
levels without insertion, removes a lower-level hit before promoting it to L1,
and moves resident victims down. A key has only one resident copy. Ghost history
in 2Q, ARC and LIRS contains no values and does not count as a resident copy.

Code: [cache_hierarchy.hpp](../include/cache_hierarchy.hpp),
[cache_hierarchy.cpp](../src/cache_hierarchy.cpp). The two-level
[multilevel_cache.hpp](../include/multilevel_cache.hpp) wrapper follows the same rule.
See [hierarchy.md](hierarchy.md) for configuration.
