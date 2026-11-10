#include <gtest/gtest.h>

#include "test.hpp"
#include "twoq_cache.hpp"

/*
 * 2Q specific tests
 */
TEST(twoq_cache_test_t, ElementStaysInA1inOnHit) {
    twoq_cache_t<int, int> cache(4);

    EXPECT_FALSE(cache.lookup_update(1, slow_get_value));

    EXPECT_TRUE(cache.lookup_update(1, slow_get_value));

    // FIXME: seems like here you access members meant to be private
    EXPECT_TRUE(cache.a1in_cache_.find(1) != cache.a1in_cache_.end());

    EXPECT_TRUE(cache.am_cache_.cache_.find(1) == cache.am_cache_.cache_.end());
}

TEST(twoq_cache_test_t, ElementMovesFromA1inToA1out) {
    twoq_cache_t<int, int> cache(4);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(2, slow_get_value);
    cache.lookup_update(3, slow_get_value);
    cache.lookup_update(4, slow_get_value);
    cache.lookup_update(5, slow_get_value);

    EXPECT_TRUE(cache.a1out_cache_.find(1) != cache.a1out_cache_.end());

    EXPECT_TRUE(cache.a1in_cache_.find(1) == cache.a1in_cache_.end());
}

TEST(twoq_cache_test_t, ElementMovesFromA1outToAm) {
    twoq_cache_t<int, int> cache(4);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(2, slow_get_value);
    cache.lookup_update(3, slow_get_value);
    cache.lookup_update(4, slow_get_value);
    cache.lookup_update(5, slow_get_value);

    ASSERT_TRUE(cache.a1out_cache_.find(1) != cache.a1out_cache_.end());

    EXPECT_FALSE(cache.lookup_update(1, slow_get_value));

    EXPECT_TRUE(cache.a1out_cache_.find(1) == cache.a1out_cache_.end());

    EXPECT_TRUE(cache.am_cache_.cache_.find(1) != cache.am_cache_.cache_.end());
}

TEST(twoq_cache_test_t, A1outDoesNotContainValues) {
    twoq_cache_t<int, int> cache(4);

    cache.lookup_update(1, slow_get_value);
    cache.lookup_update(2, slow_get_value);
    cache.lookup_update(3, slow_get_value);
    cache.lookup_update(4, slow_get_value);
    cache.lookup_update(5, slow_get_value);

    EXPECT_LE(cache.a1out_counter_.size(), cache.kout_);
}

TEST(twoq_cache_test_t, ResidentSizeDoesNotExceedCapacity) {
    twoq_cache_t<int, int> cache(4);

    for (int key = 0; key < 100; key++) {
        cache.lookup_update(key, slow_get_value);

        EXPECT_LE(cache.a1in_counter_.size() + cache.am_cache_.counter_.size(), cache.capacity_);
    }
}
