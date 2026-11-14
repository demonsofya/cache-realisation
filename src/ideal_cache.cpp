#include <cstddef>
#include <unordered_map>
#include <vector>

#include <ideal_cache.hpp>

template <typename KeyT>
size_t ideal_cache_hits(const std::vector<KeyT> &data, size_t capacity) {
    if (capacity == 0 || data.empty())
        return 0;

    std::vector<size_t> next_use(data.size(), data.size());
    std::unordered_map<KeyT, size_t> next_positions;

    for (size_t i = data.size(); i > 0;) {
        --i;
        auto next_iter = next_positions.find(data[i]);
        if (next_iter != next_positions.end())
            next_use[i] = next_iter->second;

        next_positions[data[i]] = i;
    }

    // key -> position of its next use
    std::unordered_map<KeyT, size_t> cache;
    size_t hits = 0;

    for (size_t i = 0; i < data.size(); i++) {
        auto cache_iter = cache.find(data[i]);
        if (cache_iter != cache.end()) {
            hits++;
            cache_iter->second = next_use[i];
            continue;
        }

        if (cache.size() == capacity) {
            auto deleting_iter = cache.begin();
            for (auto it = cache.begin(); it != cache.end(); it++) {
                if (it->second > deleting_iter->second)
                    deleting_iter = it;
            }

            cache.erase(deleting_iter);
        }

        cache.emplace(data[i], next_use[i]);
    }

    return hits;
}

template std::size_t ideal_cache_hits<std::size_t>(const std::vector<std::size_t> &data,
                                                   std::size_t capacity);

template std::size_t ideal_cache_hits<int>(const std::vector<int> &data, std::size_t capacity);
