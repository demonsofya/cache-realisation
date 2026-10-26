#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

#include "cache.hpp"

int slow_get_value(int key) { return key; }

struct cache_test_case_t {
  std::string name;
  size_t capacity;
  std::vector<int> data;
  size_t expected_hits;
};

const std::vector<cache_test_case_t> common_tests = {
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

template <typename Cache>
size_t run_cache(size_t capacity, const std::vector<int> &data) {
  Cache cache(capacity);

  size_t hits = 0;

  for (auto key : data) {
    if (cache.lookup_update(key, slow_get_value))
      hits++;
  }

  return hits;
}

template <typename T> class common_cache_test_t : public testing::Test {};

using CacheTypes = testing::Types<lru_cache_t<int, int>, lfu_cache_t<int, int>,
                                  twoq_cache_t<int, int>, arc_cache_t<int, int>,
                                  lirs_cache_t<int, int>>;

TYPED_TEST_SUITE(common_cache_test_t, CacheTypes);

TYPED_TEST(common_cache_test_t, CommonTests) {
  for (const auto &test : common_tests) {

    SCOPED_TRACE(test.name);

    size_t hits = run_cache<TypeParam>(test.capacity, test.data);

    EXPECT_EQ(hits, test.expected_hits);
  }
}

TYPED_TEST(common_cache_test_t, LongUniqueSequence) {
  TypeParam cache(10);

  size_t hits = 0;

  for (int key = 0; key < 1000; key++) {
    if (cache.lookup_update(key, slow_get_value))
      hits++;
  }

  EXPECT_EQ(hits, 0);
}

TYPED_TEST(common_cache_test_t, LongRepeatedElement) {
  TypeParam cache(10);

  size_t hits = 0;

  for (int i = 0; i < 1000; i++) {
    if (cache.lookup_update(1, slow_get_value))
      hits++;
  }

  EXPECT_EQ(hits, 999);
}

TYPED_TEST(common_cache_test_t, SmallCapacities) {
  for (size_t capacity = 0; capacity <= 5; capacity++) {

    TypeParam cache(capacity);

    size_t hits = 0;
    size_t slow_calls = 0;

    auto slow_get 

    for (int i = 0; i < 100; i++) {
      if(cache.lookup_update(i % 10, slow_get_value)) hits++;
    }
  }

  SUCCEED();
}

/*
 * 2Q specific tests
 */

TEST(twoq_cache_test_t, ElementStaysInA1inOnHit) {
  twoq_cache_t<int, int> cache(4);

  EXPECT_FALSE(cache.lookup_update(1, slow_get_value));

  EXPECT_TRUE(cache.lookup_update(1, slow_get_value));

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

    EXPECT_LE(cache.a1in_counter_.size() + cache.am_cache_.counter_.size(),
              cache.capacity_);
  }
}

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

    EXPECT_LE(cache.t1_counter_.size() + cache.t2_counter_.size(),
              cache.capacity_);
  }
}

TEST(arc_cache_test_t, WholeSizeDoesNotExceedDoubleCapacity) {
  arc_cache_t<int, int> cache(4);

  for (int key = 0; key < 100; key++) {
    cache.lookup_update(key, slow_get_value);

    EXPECT_LE(cache.t1_counter_.size() + cache.t2_counter_.size() +
                  cache.b1_counter_.size() + cache.b2_counter_.size(),
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

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}