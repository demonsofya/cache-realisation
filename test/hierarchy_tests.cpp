#include <gtest/gtest.h>

#include "cache.hpp"
#include "cache_hierarchy.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <random>
#include <set>

namespace {
const std::array<std::string, 5> policies{"LRU", "LFU", "2Q", "ARC", "LIRS"};
std::set<size_t> residents(const cache_hierarchy_t &cache) {
    std::set<size_t> all;
    for (size_t i = 0; i < cache.levels().size(); i++) {
        const auto keys = cache.resident_keys(i);
        EXPECT_LE(keys.size(), cache.levels()[i].config.capacity);
        for (auto key : keys)
            EXPECT_TRUE(all.insert(key).second) << "Duplicate resident key " << key;
    }
    return all;
}
} // namespace

TEST(HierarchyTest, PromotionAndVictimCascade) {
    cache_hierarchy_t cache({{"LRU", 1}, {"LRU", 2}, {"LRU", 3}});
    for (size_t key = 1; key <= 6; key++)
        EXPECT_FALSE(cache.lookup_update(key));
    EXPECT_EQ(residents(cache).size(), 6);
    EXPECT_TRUE(cache.lookup_update(1));
    EXPECT_EQ(cache.resident_keys(0), std::vector<size_t>{1});
    EXPECT_EQ(cache.levels()[2].hits, 1);
    EXPECT_EQ(residents(cache).size(), 6);
    EXPECT_FALSE(cache.lookup_update(7));
    EXPECT_EQ(residents(cache).size(), 6);
    EXPECT_EQ(cache.memory_accesses(), 7);
}

TEST(HierarchyTest, UsesAllSlotsForDistinctKeys) {
    cache_hierarchy_t cache({{"LRU", 1}, {"LRU", 2}});
    size_t hits = 0;
    for (size_t key : {1, 2, 3, 1, 2, 3})
        hits += cache.lookup_update(key);
    EXPECT_EQ(hits, 3);
    EXPECT_EQ(residents(cache), (std::set<size_t>{1, 2, 3}));
}

TEST(HierarchyTest, AllPolicyTriplesKeepKeysExclusiveAndAccountForEveryRequest) {
    for (const auto &a : policies) for (const auto &b : policies) for (const auto &c : policies) {
        SCOPED_TRACE(a + "/" + b + "/" + c);
        for (size_t base : {1, 2, 4}) {
            cache_hierarchy_t cache({{a, base}, {b, base + 1}, {c, base + 3}});
            const size_t capacity = 3 * base + 4;
            std::mt19937 random(2026);
            for (size_t request = 0; request < 1000; request++) {
                const size_t key = random() % (capacity + 7);
                auto before = residents(cache);
                bool expected_hit = before.contains(key);
                EXPECT_EQ(cache.lookup_update(key), expected_hit);
                auto after = residents(cache);
                EXPECT_TRUE(after.contains(key));
                EXPECT_EQ(after.size(), std::min(capacity, before.size() + !expected_hit));
                size_t incoming = cache.requests(), hits = 0;
                for (const auto &level : cache.levels()) {
                    EXPECT_EQ(level.hits + level.misses, incoming);
                    incoming = level.misses;
                    hits += level.hits;
                }
                EXPECT_EQ(incoming, cache.memory_accesses());
                EXPECT_EQ(hits + incoming, request + 1);
            }
        }
    }
}

TEST(HierarchyTest, ArbitraryDepthAndZeroCapacityBoundary) {
    std::vector<cache_level_config_t> config(12, {"LRU", 1});
    cache_hierarchy_t cache(config);
    for (size_t key = 0; key < 12; key++)
        EXPECT_FALSE(cache.lookup_update(key));
    for (size_t key = 0; key < 12; key++)
        EXPECT_TRUE(cache.lookup_update(key));
    EXPECT_EQ(residents(cache).size(), 12);
    EXPECT_THROW(cache_hierarchy_t({}), std::invalid_argument);
    EXPECT_THROW(cache_hierarchy_t({{"LRU", 0}, {"LFU", 2}}), std::invalid_argument);
    cache_hierarchy_t empty({{"LRU", 0}});
    EXPECT_FALSE(empty.lookup_update(1));
    EXPECT_TRUE(empty.resident_keys(0).empty());
}

template <typename Cache> class TransferTest : public testing::Test {};
using TransferTypes = testing::Types<lru_cache_t<size_t>, lfu_cache_t<size_t>,
    twoq_cache_t<size_t>, arc_cache_t<size_t>, lirs_cache_t<size_t>>;
TYPED_TEST_SUITE(TransferTest, TransferTypes);

TYPED_TEST(TransferTest, EraseAndInsertReportExactlyTheResidentVictim) {
    for (size_t capacity = 1; capacity <= 8; capacity++) {
        TypeParam cache(capacity);
        std::mt19937 random(37);
        for (size_t step = 0; step < 3000; step++) {
            const size_t key = random() % 16;
            auto keys = cache.resident_keys();
            std::set<size_t> before(keys.begin(), keys.end());
            if (step % 3 == 0) {
                EXPECT_EQ(cache.erase(key), before.erase(key) != 0);
            } else {
                std::optional<size_t> victim;
                size_t notifications = 0;
                bool hit = cache.lookup_update(key, [](size_t k) { return k; }, [&](size_t k) {
                    victim = k;
                    notifications++;
                });
                EXPECT_EQ(hit, before.contains(key));
                EXPECT_LE(notifications, 1);
                if (victim) {
                    EXPECT_EQ(before.erase(*victim), 1);
                    EXPECT_FALSE(hit);
                }
                before.insert(key);
            }
            keys = cache.resident_keys();
            EXPECT_EQ(std::set<size_t>(keys.begin(), keys.end()), before);
            EXPECT_LE(keys.size(), capacity);
            for (size_t k = 0; k < 16; k++)
                EXPECT_EQ(cache.contains(k), before.contains(k));
        }
    }
}

TYPED_TEST(TransferTest, TwoLevelWrapperPreservesResidentMembership) {
    // Every supported policy is exercised as L2, including promotion from LIRS.
    lru_cache_t<size_t> l1(2);
    TypeParam l2(5);
    multilevel_cache_t wrapper(l1, l2);
    size_t loads = 0;
    std::mt19937 random(91);
    for (size_t step = 0; step < 2000; step++) {
        size_t key = random() % 12;
        auto lower = l2.resident_keys();
        auto upper = l1.resident_keys();
        std::set<size_t> before(lower.begin(), lower.end());
        before.insert(upper.begin(), upper.end());
        EXPECT_EQ(wrapper.lookup_update(key, [&](size_t k) { loads++; return k; }), before.contains(key));
        lower = l2.resident_keys();
        upper = l1.resident_keys();
        std::set<size_t> after(lower.begin(), lower.end());
        for (auto k : upper)
            EXPECT_TRUE(after.insert(k).second);
        EXPECT_EQ(after.size(), std::min(size_t(7), before.size() + !before.contains(key)));
        EXPECT_EQ(loads, wrapper.memory_accesses_);
    }
}

TEST(HierarchyTest, WrapperRejectsPreloadedCaches) {
    lru_cache_t<size_t> l1(1), l2(2);
    l2.lookup_update(1, [](size_t k) { return k; });
    EXPECT_THROW((multilevel_cache_t<decltype(l1), decltype(l2)>(l1, l2)), std::invalid_argument);
}
