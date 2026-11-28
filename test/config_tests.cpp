#include <gtest/gtest.h>

#include "cache_config.hpp"

#include <limits>
#include <stdexcept>

namespace {
std::vector<size_t> sizes(const std::vector<cache_level_config_t> &levels) {
    std::vector<size_t> result;
    for (const auto &level : levels)
        result.push_back(level.capacity);
    return result;
}
} // namespace

TEST(ConfigTest, DistributionsPreserveBudget) {
    const std::vector<std::string> policies{"LRU", "ARC", "LIRS"};
    EXPECT_EQ(sizes(make_levels(policies, 21, "geometric")), (std::vector<size_t>{3, 6, 12}));
    EXPECT_EQ(sizes(make_levels(policies, 21, "equal")), (std::vector<size_t>{7, 7, 7}));
    EXPECT_EQ(sizes(make_levels(policies, 21, "linear")), (std::vector<size_t>{3, 7, 11}));
    EXPECT_EQ(sizes(make_levels(policies, 22, "geometric")), (std::vector<size_t>{3, 6, 13}));
    EXPECT_EQ(sizes(make_levels(policies, 21, "geometric", 4)), (std::vector<size_t>{1, 4, 16}));
    EXPECT_EQ(sizes(make_levels(policies, 21, "geometric", 2, {1, 5, 15})),
              (std::vector<size_t>{1, 5, 15}));
}

TEST(ConfigTest, RejectsInvalidSizesAndOverflow) {
    const std::vector<std::string> two{"LRU", "LFU"};
    EXPECT_THROW(make_levels({}, 10), std::invalid_argument);
    EXPECT_THROW(make_levels(two, 2), std::invalid_argument);
    EXPECT_THROW(make_levels(two, 10, "geometric", 1), std::invalid_argument);
    EXPECT_THROW(make_levels(two, 10, "unknown"), std::invalid_argument);
    EXPECT_THROW(make_levels(two, 10, "equal", 2, {5}), std::invalid_argument);
    EXPECT_THROW(make_levels(two, 10, "equal", 2, {1, 8}), std::invalid_argument);
    EXPECT_THROW(make_levels(two, 10, "equal", 2, {0, 10}), std::invalid_argument);
    EXPECT_THROW(make_levels(std::vector<std::string>(100, "LRU"), 1000), std::overflow_error);
    EXPECT_THROW(make_levels(two, 10, "equal", 2, {std::numeric_limits<size_t>::max(), 1}),
                 std::overflow_error);
    EXPECT_EQ(sizes(make_levels({"LRU"}, 0)), (std::vector<size_t>{0}));
    EXPECT_EQ(sizes(make_levels({"LRU"}, std::numeric_limits<size_t>::max())),
              (std::vector<size_t>{std::numeric_limits<size_t>::max()}));
}
