#include <gtest/gtest.h>

#include "arc_cache.hpp"
#include "test.hpp"

/*
 * ARC specific tests
 */
TEST(arc_cache_test_t, NewElementGoesToT1) {
    arc_cache_t<int, int> cache(4);

    EXPECT_FALSE(cache.lookup_update(1, slow_get_value));

    EXPECT_TRUE(cache.t1_cache_.find(1) != cache.t1_cache_.end());

    EXPECT_TRUE(cache.t2_cache_.find(1) == cache.t2_cache_.end());
}

TEST(arc_cache_test_t, ElementMovesFromT1ToT2) {
    arc_cache_t<int, int> cache(4);

    cache.lookup_update(1, slow_get_value);

    ASSERT_TRUE(cache.t1_cache_.find(1) != cache.t1_cache_.end());

    EXPECT_TRUE(cache.lookup_update(1, slow_get_value));

    EXPECT_TRUE(cache.t1_cache_.find(1) == cache.t1_cache_.end());

    EXPECT_TRUE(cache.t2_cache_.find(1) != cache.t2_cache_.end());
}

TEST(arc_cache_test_t, HitInT2StaysInT2) {
    arc_cache_t<int, int> cache(4);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(1, slow_get_value);

    ASSERT_TRUE(cache.t2_cache_.find(1) != cache.t2_cache_.end());

    EXPECT_TRUE(cache.lookup_update(1, slow_get_value));

    EXPECT_TRUE(cache.t2_cache_.find(1) != cache.t2_cache_.end());
}

TEST(arc_cache_test_t, T1EvictionGoesToB1) {
    arc_cache_t<int, int> cache(2);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(2, slow_get_value);

    cache.lookup_update(1, slow_get_value);

    cache.lookup_update(3, slow_get_value);

    EXPECT_TRUE(cache.b1_cache_.find(2) != cache.b1_cache_.end());
}

TEST(arc_cache_test_t, B1HitIncreasesP) {
    arc_cache_t<int, int> cache(2);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(2, slow_get_value);
    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(3, slow_get_value);

    ASSERT_TRUE(cache.b1_cache_.find(2) != cache.b1_cache_.end());

    size_t old_p = cache.p_;

    EXPECT_FALSE(cache.lookup_update(2, slow_get_value));

    EXPECT_GT(cache.p_, old_p);

    EXPECT_TRUE(cache.t2_cache_.find(2) != cache.t2_cache_.end());
}

TEST(arc_cache_test_t, B2HitDecreasesP) {
    arc_cache_t<int, int> cache(2);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(2, slow_get_value);
    cache.lookup_update(1, slow_get_value);

    cache.lookup_update(3, slow_get_value);

    cache.lookup_update(2, slow_get_value);

    ASSERT_TRUE(cache.b2_cache_.find(1) != cache.b2_cache_.end());

    size_t old_p = cache.p_;

    EXPECT_FALSE(cache.lookup_update(1, slow_get_value));

    EXPECT_LT(cache.p_, old_p);

    EXPECT_TRUE(cache.t2_cache_.find(1) != cache.t2_cache_.end());
}

TEST(arc_cache_test_t, ResidentSizeDoesNotExceedCapacity) {
    arc_cache_t<int, int> cache(4);

    for (int key = 0; key < 100; key++) {
        cache.lookup_update(key, slow_get_value);

        EXPECT_LE(cache.t1_counter_.size() + cache.t2_counter_.size(), cache.capacity_);
    }
}

TEST(arc_cache_test_t, WholeSizeDoesNotExceedDoubleCapacity) {
    arc_cache_t<int, int> cache(4);

    for (int key = 0; key < 100; key++) {
        cache.lookup_update(key, slow_get_value);

        EXPECT_LE(cache.t1_counter_.size() + cache.t2_counter_.size() + cache.b1_counter_.size() +
                      cache.b2_counter_.size(),
                  2 * cache.capacity_);
    }
}

TEST(arc_cache_test_t, PDoesNotExceedCapacity) {
    arc_cache_t<int, int> cache(4);

    for (int i = 0; i < 100; i++) {
        cache.lookup_update(i % 10, slow_get_value);

        EXPECT_LE(cache.p_, cache.capacity_);
    }
}
