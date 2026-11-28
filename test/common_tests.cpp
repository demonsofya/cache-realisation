#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

#include "cache.hpp"

#include "test.hpp"

struct cache_test_case_t {
    std::string name;
    size_t capacity;
    std::vector<int> data;
    size_t expected_hits;
};

template <typename Cache>
size_t run_cache(size_t capacity, const std::vector<int> &data) {
    Cache cache(capacity);

    size_t hits = 0;

    for (auto key : data) {
        hits += cache.lookup_update(key, slow_get_value);
    }

    return hits;
}

template <typename T>
class CommonCacheTest : public testing::Test {};

using CacheTypes =
    testing::Types<lru_cache_t<int, int>, lfu_cache_t<int, int>, twoq_cache_t<int, int>,
                   arc_cache_t<int, int>, lirs_cache_t<int, int>>;

TYPED_TEST_SUITE(CommonCacheTest, CacheTypes);

TYPED_TEST(CommonCacheTest, CommonTests) {

    const std::vector<cache_test_case_t> common_tests{
        {"zero capacity", 0, {1, 1, 1}, 0},
        {"empty input", 4, {}, 0},
        {"single element", 4, {1}, 0},
        {"single element repeated", 4, {1, 1, 1, 1}, 3},
        {"capacity one repeated", 1, {1, 1, 1, 1}, 3},
        {"capacity one changing", 1, {1, 2, 3, 4}, 0},
        {"capacity one pairs", 1, {1, 1, 2, 2, 3, 3}, 3},
        {"all unique fit", 4, {1, 2, 3, 4}, 0},
        {"all unique overflow", 3, {1, 2, 3, 4, 5, 6}, 0},
        {"two elements fit", 2, {1, 2, 1, 2}, 2},
        {"two elements repeated", 2, {1, 2, 1, 2, 1, 2}, 4},
        {"three elements fit", 3, {1, 2, 3, 1, 2, 3}, 3},
        {"four elements fit", 4, {1, 2, 3, 4, 1, 2, 3, 4}, 4},
        {"multiple repetitions", 4, {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4}, 8},
        {"zero key", 3, {0, 0, 0}, 2},
        {"negative keys", 3, {-1, -2, -3, -1, -2, -3}, 3},
        {"large keys", 3, {100000, 200000, 300000, 100000, 200000, 300000}, 3},
        {"repeated last element", 3, {1, 2, 3, 3, 3, 3}, 3}};

    for (const auto &test : common_tests) {

        SCOPED_TRACE(test.name);

        size_t hits = run_cache<TypeParam>(test.capacity, test.data);

        EXPECT_EQ(hits, test.expected_hits);
    }
}

TYPED_TEST(CommonCacheTest, LongUniqueSequence) {
    TypeParam cache(10);

    size_t hits = 0;

    for (int key = 0; key < 1000; key++) {
        hits += cache.lookup_update(key, slow_get_value);
    }

    EXPECT_EQ(hits, 0);
}

TYPED_TEST(CommonCacheTest, LongRepeatedElement) {
    TypeParam cache(10);

    size_t hits = 0;

    for (int i = 0; i < 1000; i++) {
        hits += cache.lookup_update(1, slow_get_value);
    }

    EXPECT_EQ(hits, 999);
}

TYPED_TEST(CommonCacheTest, SmallCapacities) {
    for (size_t capacity = 1; capacity <= 5; capacity++) {
        TypeParam cache(capacity);

        EXPECT_FALSE(cache.lookup_update(1, slow_get_value));
        EXPECT_TRUE(cache.lookup_update(1, slow_get_value));
    }
}
