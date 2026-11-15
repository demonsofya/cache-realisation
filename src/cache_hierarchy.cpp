#include "cache_hierarchy.hpp"
#include "cache.hpp"

#include <stdexcept>

struct cache_hierarchy_t::level_t {
    virtual ~level_t() = default;
    virtual bool lookup_update(size_t key) = 0;
};

template <typename Cache>
struct cache_hierarchy_t::level_impl_t : level_t {
    explicit level_impl_t(size_t capacity) : cache(capacity) {}

    bool lookup_update(size_t key) override {
        return cache.lookup_update(key, [](size_t requested_key) { return requested_key; });
    }

    Cache cache;
};

cache_hierarchy_t::cache_hierarchy_t(const std::vector<cache_level_config_t> &config) {
    if (config.empty())
        throw std::invalid_argument("At least one cache level is required");

    for (const auto &level : config) {
        if (level.policy == "LRU")
            caches_.push_back(std::make_unique<level_impl_t<lru_cache_t<size_t>>>(level.capacity));
        else if (level.policy == "LFU")
            caches_.push_back(std::make_unique<level_impl_t<lfu_cache_t<size_t>>>(level.capacity));
        else if (level.policy == "2Q")
            caches_.push_back(std::make_unique<level_impl_t<twoq_cache_t<size_t>>>(level.capacity));
        else if (level.policy == "ARC")
            caches_.push_back(std::make_unique<level_impl_t<arc_cache_t<size_t>>>(level.capacity));
        else if (level.policy == "LIRS")
            caches_.push_back(std::make_unique<level_impl_t<lirs_cache_t<size_t>>>(level.capacity));
        else
            throw std::invalid_argument("Unknown cache policy: " + level.policy);
        stats_.push_back({level});
    }
}

cache_hierarchy_t::~cache_hierarchy_t() = default;

bool cache_hierarchy_t::lookup_update(size_t key) {
    requests_++;
    for (size_t i = 0; i < caches_.size(); i++) {
        if (caches_[i]->lookup_update(key)) {
            stats_[i].hits++;
            return true;
        }
        stats_[i].misses++;
    }
    memory_accesses_++;
    return false;
}

const std::vector<cache_level_stats_t> &cache_hierarchy_t::levels() const {
    return stats_;
}

size_t cache_hierarchy_t::requests() const {
    return requests_;
}

size_t cache_hierarchy_t::memory_accesses() const {
    return memory_accesses_;
}
