#include <gtest/gtest.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "cache.hpp"

namespace {

int slow_get_value(int key) { return key; }

void print_result(const char *name, size_t requests, size_t hits) {
    std::cout << std::left << std::setw(18) << name << std::right << std::setw(10) << hits
              << std::setw(10) << requests - hits << '\n';
}

template <typename Cache>
size_t count_hits(Cache &cache, const std::vector<int> &data) {
    size_t hits = 0;
    for (int key : data)
        hits += cache.lookup_update(key, slow_get_value);
    return hits;
}

void compare_caches(size_t capacity, const std::vector<int> &data,
                    const char *description = "Cache comparison") {
    lru_cache_t<int, int> lru(capacity);
    lfu_cache_t<int, int> lfu(capacity);
    twoq_cache_t<int, int> twoq(capacity);
    arc_cache_t<int, int> arc(capacity);
    lirs_cache_t<int, int> lirs(capacity);

    const size_t ideal_hits = ideal_cache_hits(data, capacity);
    const size_t l1_capacity = capacity / 3;
    const size_t l2_capacity = capacity - l1_capacity;

    std::cout << "\nScenario: " << description << "\nCapacity: " << capacity
              << "\nRequests: " << data.size() << "\nData:";
    const size_t preview_size = std::min(data.size(), size_t{40});
    for (size_t i = 0; i < preview_size; i++)
        std::cout << ' ' << data[i];
    if (preview_size < data.size())
        std::cout << " ...";
    std::cout << '\n';
    if (l1_capacity > 0)
        std::cout << "L1 (LRU): " << l1_capacity << ", L2 (LFU): " << l2_capacity << '\n';

    std::cout << '\n'
              << std::left << std::setw(18) << "Cache" << std::right << std::setw(10) << "Hits"
              << std::setw(10) << "Misses" << '\n';

    auto check_cache = [&](const char *name, auto &cache) {
        const size_t hits = count_hits(cache, data);
        print_result(name, data.size(), hits);
        EXPECT_LE(hits, ideal_hits) << name;
    };

    check_cache("LRU", lru);
    check_cache("LFU", lfu);
    check_cache("2Q", twoq);
    check_cache("ARC", arc);
    check_cache("LIRS", lirs);
    print_result("IDEAL", data.size(), ideal_hits);

    if (l1_capacity > 0) {
        lru_cache_t<int, int> l1(l1_capacity);
        lfu_cache_t<int, int> l2(l2_capacity);
        multilevel_cache_t multilevel(l1, l2);

        const size_t hits = count_hits(multilevel, data);
        print_result("L1+L2 (LRU+LFU)", data.size(), hits);
        EXPECT_LE(hits, ideal_hits);
        EXPECT_EQ(hits, multilevel.hits_l1_ + multilevel.hits_l2_);
        EXPECT_EQ(data.size(), hits + multilevel.memory_accesses_);

        std::cout << "L1 hits: " << multilevel.hits_l1_ << ", L2 hits: " << multilevel.hits_l2_
                  << '\n';
    }
}

void append_repeated(std::vector<int> &data, const std::vector<int> &pattern, size_t count) {
    for (size_t i = 0; i < count; i++)
        data.insert(data.end(), pattern.begin(), pattern.end());
}

void append_scan(std::vector<int> &data, int first, int count) {
    for (int key = first; key < first + count; key++)
        data.push_back(key);
}

} // namespace

TEST(CacheComparison, EmptyInput) { compare_caches(6, {}, "Empty request stream"); }

TEST(CacheComparison, ZeroCapacity) {
    compare_caches(0, {1, 2, 1, 2, 3, 1}, "Cache disabled: zero capacity");
}

TEST(CacheComparison, CapacityOne) {
    compare_caches(1, {1, 1, 2, 2, 1, 3, 3, 3, 1}, "Repeated accesses with capacity one");
}

TEST(CacheComparison, CapacityTwo) {
    compare_caches(2, {1, 2, 3, 1, 2, 3, 1, 2, 3}, "Three keys competing for two slots");
}

TEST(CacheComparison, SmallestMultilevel) {
    compare_caches(3, {1, 1, 2, 1, 3, 1, 2, 2, 4, 1}, "Smallest two-level cache");
}

TEST(CacheComparison, CapacityNotDivisibleByThree) {
    compare_caches(5, {1, 2, 3, 1, 2, 4, 5, 6, 1, 2, 3, 4, 5, 6},
                   "Cache capacity not divisible by three");
}

TEST(CacheComparison, HotKeysAndScans) {
    compare_caches(6, {1, 2, 1, 2, 1,  2,  3,  4,  5,  6, 7, 8, 1, 2, 1,
                       2, 1, 2, 9, 10, 11, 12, 13, 14, 1, 2, 1, 2, 1, 2},
                   "Popular keys between sequential scans");
}

TEST(CacheComparison, WorkingSetFits) {
    compare_caches(9, {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6},
                   "Working set fits in the cache");
}

TEST(CacheComparison, ChangingWorkingSet) {
    compare_caches(6, {1, 2, 3, 1, 2, 3, 1, 2, 3, 4, 5, 6, 4, 5, 6, 4, 5, 6, 1, 2, 3, 1, 2, 3},
                   "Alternating working sets");
}

TEST(CacheComparison, UniqueKeys) {
    compare_caches(12, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   "One-time accesses to unique data");
}

// Synthetic read-only traces inspired by locality, popularity and changing workloads:
// https://www.usenix.org/conference/osdi20/presentation/yang
// https://www.usenix.org/conference/fast20/presentation/cao-zhichao
// https://www.usenix.org/conference/fast-03/arc-self-tuning-low-overhead-replacement-cache
// These are illustrative scenarios, not production traces or models of TTL/writes.

TEST(CacheComparison, RepeatedData) {
    std::vector<int> data;
    append_repeated(data, {1, 2, 3}, 30);
    compare_caches(6, data, "Repeated data: revisiting the same three pages");
}

TEST(CacheComparison, FrequentlyAccessedItem) {
    std::vector<int> data;
    append_repeated(data, {1}, 12);
    for (int first = 10; first < 90; first += 8) {
        append_scan(data, first, 8);
        data.push_back(1);
    }
    compare_caches(6, data, "Frequently accessed item among new pages");
}

TEST(CacheComparison, CatalogScanAndFavorites) {
    std::vector<int> data;
    append_repeated(data, {1, 2, 3}, 10);
    for (int first = 100; first < 180; first += 20) {
        append_scan(data, first, 20);
        append_repeated(data, {1, 2, 3}, 3);
    }
    compare_caches(9, data, "Browsing a catalog and returning to favorites");
}

TEST(CacheComparison, SequentialReading) {
    std::vector<int> data;
    append_scan(data, 1, 120);
    compare_caches(12, data, "Sequential reading without revisits");
}

TEST(CacheComparison, CyclicWorkingSetExceedsCache) {
    std::vector<int> data;
    append_repeated(data, {1, 2, 3, 4, 5, 6, 7}, 20);
    compare_caches(6, data, "Cycling through seven pages with capacity six");
}

TEST(CacheComparison, SwitchingInterests) {
    std::vector<int> data;
    append_repeated(data, {1, 2, 3, 4, 5, 6}, 20);
    append_repeated(data, {11, 12, 13, 14, 15, 16}, 30);
    compare_caches(6, data, "Switching interests: previously popular data is no longer needed");
}

TEST(CacheComparison, ReturningToPreviousProject) {
    std::vector<int> data;
    append_repeated(data, {1, 2, 3, 4}, 15);
    append_repeated(data, {11, 12, 13, 14}, 15);
    append_repeated(data, {1, 2, 3, 4}, 15);
    compare_caches(6, data, "Switching projects and returning to the previous one");
}

TEST(CacheComparison, ShortBurstsOfNewData) {
    std::vector<int> data;
    for (int key = 1; key <= 40; key++)
        append_repeated(data, {key}, 5);
    compare_caches(6, data, "Short bursts: access an item repeatedly, then move on");
}

TEST(CacheComparison, MovingWorkingWindow) {
    std::vector<int> data;
    for (int first = 1; first <= 20; first++)
        append_repeated(data, {first, first + 1, first + 2, first + 3}, 3);
    compare_caches(6, data, "Scrolling with revisits: a gradually moving working window");
}

TEST(CacheComparison, PopularitySkewWithLongTail) {
    // Integer weights approximate popularity proportional to 1 / rank.
    std::vector<int> population;
    for (int key = 1; key <= 32; key++)
        population.insert(population.end(), 320 / key, key);

    std::mt19937 generator(2026);
    std::vector<int> data;
    for (size_t i = 0; i < 1000; i++)
        data.push_back(population[generator() % population.size()]);

    for (size_t capacity : {6u, 12u, 24u})
        compare_caches(capacity, data, "Popular pages and a long tail of rare requests");
}

TEST(CacheComparison, UniformRandomRequests) {
    std::mt19937 generator(2026);
    std::vector<int> data;
    for (size_t i = 0; i < 1000; i++)
        data.push_back(static_cast<int>(generator() % 32) + 1);
    compare_caches(12, data, "Random requests with roughly equal page popularity");
}

TEST(CacheComparison, InterleavedUserSessions) {
    std::vector<int> data;
    for (int session = 0; session < 12; session++) {
        const int first = 100 + session * 10;
        for (int step = 0; step < 5; step++) {
            // A regular user revisits the same pages while a new visitor browses.
            append_repeated(data, {1, 2, 3}, 1);
            data.push_back(first + step);
            data.push_back(first + step);
        }
    }
    compare_caches(9, data, "Interleaved sessions: a regular user and new visitors");
}
