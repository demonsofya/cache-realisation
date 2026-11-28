#include "cache_hierarchy.hpp"
#include "cache.hpp"

#include <stdexcept>
#include <optional>

struct cache_hierarchy_t::level_t {
    virtual ~level_t() = default;
    virtual bool lookup_update(size_t key) = 0;
    virtual bool contains(size_t key) const = 0;
    virtual void erase(size_t key) = 0;
    virtual std::optional<size_t> insert(size_t key) = 0;
    virtual std::vector<size_t> resident_keys() const = 0;
};

template <typename Cache>
struct cache_hierarchy_t::level_impl_t : level_t {
    explicit level_impl_t(size_t capacity) : cache(capacity) {}

    bool lookup_update(size_t key) override {
        return cache.lookup_update(key, [](size_t requested_key) { return requested_key; });
    }

    bool contains(size_t key) const override { return cache.contains(key); }
    void erase(size_t key) override { cache.erase(key); }
    std::vector<size_t> resident_keys() const override { return cache.resident_keys(); }

    std::optional<size_t> insert(size_t key) override {
        std::optional<size_t> victim;
        cache.lookup_update(key, [](size_t key) { return key; },
                            [&](size_t evicted) { victim = evicted; });
        return victim;
    }

    Cache cache;
};

cache_hierarchy_t::cache_hierarchy_t(const std::vector<cache_level_config_t> &config) {
    if (config.empty())
        throw std::invalid_argument("At least one cache level is required");

    for (const auto &level : config) {
        if (config.size() > 1 && level.capacity == 0)
            throw std::invalid_argument("Every hierarchy level must have positive capacity");
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

void cache_hierarchy_t::insert(size_t key) {
    for (auto &level : caches_) {
        auto victim = level->insert(key);
        if (!victim)
            return;
        key = *victim;
    }
}

bool cache_hierarchy_t::lookup_update(size_t key) {
    requests_++;
    for (size_t i = 0; i < caches_.size(); i++) {
        if (caches_[i]->contains(key)) {
            stats_[i].hits++;
            if (i == 0) {
                caches_[0]->lookup_update(key);
            } else {
                // Remove the source before promotion, then cascade resident victims down.
                caches_[i]->erase(key);
                insert(key);
            }
            return true;
        }
        stats_[i].misses++;
    }
    memory_accesses_++;
    insert(key);
    return false;
}

std::vector<size_t> cache_hierarchy_t::resident_keys(size_t level) const {
    return caches_.at(level)->resident_keys();
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
