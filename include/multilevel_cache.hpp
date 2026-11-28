#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>

// Exclusive hit-count simulator: values must equal keys.
// Both initially empty caches must outlive this object and be accessed only through it.
template <typename L1Cache, typename L2Cache>
class multilevel_cache_t {
  public:
    multilevel_cache_t(L1Cache &l1, L2Cache &l2) : l1_(l1), l2_(l2) {
        if (l1_.capacity() == 0 || l1_.capacity() >= l2_.capacity())
            throw std::invalid_argument("L1 capacity must be positive and smaller than L2 capacity");
        if (!l1_.resident_keys().empty() || !l2_.resident_keys().empty())
            throw std::invalid_argument("The supplied caches must initially be empty");
    }

    template <typename KeyT, typename F>
    bool lookup_update(KeyT key, F slow_get_value) {
        if (l1_.contains(key)) {
            hits_l1_++;
            l1_.lookup_update(key, slow_get_value);
            return true;
        }
        bool hit = l2_.contains(key);
        if (hit) {
            hits_l2_++;
            l2_.erase(key);
        } else {
            // Loading happens once per hierarchy miss.
            slow_get_value(key);
            memory_accesses_++;
        }
        std::optional<KeyT> victim;
        l1_.lookup_update(key, [](KeyT k) { return k; },
                          [&](KeyT evicted) { victim = evicted; });
        if (victim)
            l2_.lookup_update(*victim, [](KeyT k) { return k; });
        return hit;
    }

    size_t hits_l1_ = 0;
    size_t hits_l2_ = 0;
    size_t memory_accesses_ = 0;

  private:
    L1Cache &l1_;
    L2Cache &l2_;
};
