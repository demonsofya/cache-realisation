#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "ideal_cache.hpp"

namespace {

size_t min_misses(const std::vector<int> &data, size_t position, size_t capacity,
                  std::vector<int> resident) {
    if (position == data.size())
        return 0;

    int key = data[position];
    if (std::find(resident.begin(), resident.end(), key) != resident.end())
        return min_misses(data, position + 1, capacity, resident);

    if (capacity == 0)
        return data.size() - position;

    if (resident.size() < capacity) {
        resident.push_back(key);
        return 1 + min_misses(data, position + 1, capacity, resident);
    }

    size_t best = data.size();
    for (size_t i = 0; i < resident.size(); i++) {
        auto next = resident;
        next[i] = key;
        best = std::min(best, 1 + min_misses(data, position + 1, capacity, next));
    }
    return best;
}

} // namespace

TEST(IdealCacheTest, MatchesExhaustiveOptimalEvictions) {
    for (size_t length = 0, variants = 1; length <= 7; length++, variants *= 3) {
        for (size_t code = 0; code < variants; code++) {
            std::vector<int> data(length);
            size_t value = code;
            for (auto &key : data) {
                key = static_cast<int>(value % 3);
                value /= 3;
            }
            for (size_t capacity = 0; capacity <= 3; capacity++) {
                size_t hits = ideal_cache_hits(data, capacity);

                EXPECT_EQ(data.size() - hits, min_misses(data, 0, capacity, {}));
            }
        }
    }
}

TEST(IdealCacheTest, RepeatedCallsAreIndependent) {
    const std::vector<int> data{1, 2, 3, 1, 2, 3};
    EXPECT_EQ(ideal_cache_hits(data, 2), 2);
    EXPECT_EQ(ideal_cache_hits(data, 1), 0);
    EXPECT_EQ(ideal_cache_hits(data, 2), 2);
}
