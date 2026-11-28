# Cache descriptions: detailed version

[Usage](../README.md) | [Short version](cache_summary.md) | [Results](results.md)

This guide describes the code at commit `8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6`. The GitHub links point to that revision, so their line numbers remain stable.

## Shared interface

LRU, LFU, 2Q, ARC and LIRS have a method with this shape:

```cpp
template <typename F>
bool lookup_update(KeyT key, F slow_get_value);
```

It returns `true` when a value is already stored and `false` when it must be loaded. On a miss, the callback provides the value. With zero capacity these classes return `false` without calling the callback.

A **resident** entry has a stored value. A **history** or **ghost** entry keeps only a key after its value has been removed. Finding a key in history is still a cache miss.

The classes use templates for values and keys. The command-line runner uses `size_t` keys and values, with `value == key`. The typed GoogleTest cases also use `int`, including negative keys; negative keys are not accepted by the runner's input parser.

## LRU

Code: [lru_cache_t::lookup_update](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/lru_cache.hpp#L13).

### Stored state

- `counter_`: a list of `(key, value)` pairs. The front is the most recently used entry.
- `cache_`: a hash map from each key to its list iterator.
- `size_` and `capacity_`: the current and maximum number of entries.

### Request handling

On a hit, the map provides the iterator. `counter_.splice` moves that entry to the front without copying the value or creating a new list node.

On a miss, the callback loads the value. If the cache is full, the last entry is removed from both the map and the list. The new entry is inserted at the front, and the map stores its iterator.

### Behaviour and cost

LRU adapts quickly when the working set changes. A long sequence of new keys can remove useful entries, even if they were used many times before. A repeated cycle of C+1 keys can miss on every request with capacity C.

List moves are constant-time. With average-case hash lookup, a request takes expected O(1) time. Storage is O(C). This is not a worst-case guarantee for a hash table.

## LFU

Code: [lfu_cache_t::lookup_update](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/lfu_cache.hpp#L13).

### Stored state

`cache_` maps keys to values. `counter_` is a list of `(key, frequency)` pairs ordered from lower to higher frequency.

On a hit, the code first finds the value in the map, then scans the list to find the frequency entry. It increments the frequency, searches for the new position and moves the node with `splice`.

On a miss, a new entry starts with count 1. If there is free space, it is inserted at the front. Otherwise, the first entry is replaced.

### Details that affect results

This implementation does not reduce old counts over time. Old popular keys can stay in the cache after the user has moved to a new set of data. Equal counts are handled by the list's insertion and movement order, not by a separate LRU tie-break rule.

LFU can protect popular objects from one-time requests. It can also perform poorly after a change of interests. The implementation scans the list on a hit, so the update can take O(C) time. It should not be described as an O(1) LFU implementation.

## 2Q

Code: [lookup_update](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/twoq_cache.hpp#L14) and [reclaim](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/twoq_cache.hpp#L55).

### Three parts

| Part | Contents | Role |
|---|---|---|
| `a1in_counter_` and `a1in_cache_` | Keys and values | New objects |
| `a1out_counter_` and `a1out_cache_` | Keys only | Recently removed objects |
| `am_cache_` | An `lru_cache_t` | Objects that returned from A1out |

For positive capacity, `kin_ = max(1, C / 4)` and `kout_ = max(1, C / 2)`. `kin_` is used when choosing a victim; it is not a strict size limit during the initial fill. `kout_` limits history size.

### Request handling

1. A hit in Am updates its LRU order.
2. A hit in A1in returns `true` without moving the entry to Am.
3. A match in A1out removes the history entry, makes room and loads the object into Am. It returns `false` because the value was not stored.
4. A completely new object is loaded into A1in.

When full, `reclaim()` removes an A1in entry if A1in is above its target or Am is empty. That key is added to A1out. Otherwise, the least recently used Am entry is removed.

A1in and Am share the total capacity through `size_` and `reclaim()`. Although the inner LRU was constructed with capacity C, it is not an extra independent C-slot cache.

This design separates first visits from later returns. Its behaviour depends on queue sizes and on whether keys return before they leave the history list. Hash and list operations are expected O(1) for the normal request path, with O(C) total resident and history storage.

## ARC

Code: [lookup_update](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/arc_cache.hpp#L54) and [replace](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/arc_cache.hpp#L28).

ARC stores values in T1 and T2 and removed keys in B1 and B2. Each list has a matching hash map of iterators.

| Request location | Action | Result |
|---|---|---|
| T1 | Move the value to the front of T2 | Hit |
| T2 | Move it to the front of T2 | Hit |
| B1 | Increase `p_`, replace an entry, reload into T2 | Miss |
| B2 | Decrease `p_`, replace an entry, reload into T2 | Miss |
| Nowhere | Make room and insert into T1 | Miss |

`p_` is the target size for the recent-entry part T1. A B1 match suggests that recent objects need more space. A B2 match suggests that reused objects need more space. The changes use ratios of the history-list sizes, with a minimum step of 1.

`replace(key)` decides whether to remove the last T1 or T2 entry and records the key in the matching history list. New-entry handling also limits total history.

ARC adapts its balance instead of using one fixed split. This does not mean it wins on every trace: in the saved cycle-of-seven scenario, this implementation has the same 140 misses as LRU. Its regular request path uses expected O(1) hash/list operations and O(C) state, including bounded history.

## LIRS

Code: [lookup_update](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/lirs_cache.hpp#L13), [evict_hir](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/lirs_cache.hpp#L178), [demote_bottom_lir](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/lirs_cache.hpp#L203) and [prune_stack](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/include/lirs_cache.hpp#L223).

### Stored state

- `stack_` is stack S, which tracks access history.
- `queue_` is queue Q, which holds resident HIR keys.
- `cache_` contains resident values.
- `pages_` contains status, flags and iterators for known keys.

The statuses are `LIR`, `RESIDENT_HIR` and `NON_RESIDENT_HIR`. LIR entries form the protected group. For positive C, the HIR target `lhirs_` is `max(1, C / 4)`; the LIR target `llirs_` is the remaining capacity.

### Request handling

New keys first fill the LIR group. Later new keys enter as resident HIR. A full HIR queue removes its last entry.

A LIR hit moves the key to the top of S. A resident HIR hit promotes the key to LIR if it is still in S; an old LIR key is then demoted to HIR. If the HIR key is only in Q, it stays HIR and is added back to S.

A non-resident HIR match is a miss: the value is loaded again. The key then becomes LIR, and another LIR key is demoted.

`prune_stack()` removes HIR entries from the bottom until the bottom is LIR. Resident HIR entries can remain only in Q. A non-resident entry removed from S is forgotten.

### Important limits

This implementation records history in S and `pages_`. It does not set a separate hard limit on all non-resident history entries, so total metadata should not be described as always bounded by C. One pruning call can remove several entries; not every request has a fixed amount of list work.

LIRS can protect reused objects during scans and changes of the current window. Its HIR/LIR split and history handling still affect the measured result.

## IDEAL

Code: [declaration](../include/ideal_cache.hpp), [ideal_cache_hits](https://github.com/demonsofya/cache-realisation/blob/8d147bb2a0cd85d7b0bb9c708c190fb93a5bead6/src/ideal_cache.cpp#L8), and Python `ideal_hits` in [compare_caches.py](../scripts/compare_caches.py).

The function takes the complete request vector and capacity, and returns the number of hits. It does not use `lookup_update`.

A backward pass fills `next_use`: the index of the next occurrence of each request. `data.size()` means there is no later use. `next_positions` stores the nearest future occurrence seen so far.

A forward pass maintains a map from each resident key to its next-use position. A hit updates that position and increments the count. On a full-cache miss, the code scans resident entries and removes the one with the largest next-use position.

This is Belady's rule for equal-size objects in an initially empty demand-loaded cache. It is an offline reference: real online policies cannot read future requests.

The C++ definition is in a `.cpp` file. Explicit instantiations at its end provide versions for `size_t` and `int`; a new key type needs another instantiation or a visible template definition.

For N requests and capacity C, the implementation has expected O(N) preprocessing and up to O(N*C) work during replacement. Memory includes O(N) next-use information. The Python comparison uses its own implementation of the same rule; it does not call the C++ IDEAL function.

## Cache hierarchies

Code: [cache_hierarchy.hpp](../include/cache_hierarchy.hpp),
[cache_hierarchy.cpp](../src/cache_hierarchy.cpp).

The hierarchy owns policy adapters through `unique_ptr<level_t>`. A request first
probes resident presence with `contains`. An L1 hit updates local replacement
state. A lower-level hit is erased from its source, then inserted into L1. A full
miss increments the memory counter and inserts into L1. Eviction callbacks report
the resident victim to insert into the next level. Transfers do not count as user
requests. Only a victim leaving the final level is discarded.

Exclusivity applies to resident values. Ghost metadata may mention a key resident
elsewhere. `erase` removes the promoted object without making a new ghost.
The destination uses its own admission rules; LFU counts start at 1.
ARC skips replacement if a free resident slot exists. LIRS maintains its LIR count
on removal and demotes only when the LIR target would be exceeded.

The two-level `multilevel_cache_t` wrapper also uses exclusive placement. It
requires initially empty referenced caches, positive L1 capacity smaller than L2,
and access through the wrapper for its lifetime. Both interfaces simulate values
using their keys, rather than moving arbitrary payloads.

See [hierarchy.md](hierarchy.md) for allocation and examples.

## Where correctness is checked

- [common_tests.cpp](../test/common_tests.cpp): shared cases for all five online policies.
- [twoq_tests.cpp](../test/twoq_tests.cpp): queue movement and resident/history sizes.
- [arc_tests.cpp](../test/arc_tests.cpp): list movement, target changes and size limits.
- [ideal_tests.cpp](../test/ideal_tests.cpp): exhaustive small cases against a separate recursive minimum-miss calculation.
- [hierarchy_tests.cpp](../test/hierarchy_tests.cpp): exclusive placement, victim transfers and all 125 policy triples.
- [test_cache_runner.py](../scripts/test_cache_runner.py): input/output checks and an independent Python LRU hierarchy.
- [compare_caches.py](../scripts/compare_caches.py): comparison tables, IDEAL bounds and counter checks.

Passing these checks supports the tested behaviour; it is not a proof for every possible sequence.
