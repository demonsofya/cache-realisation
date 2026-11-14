#pragma once

#include <cstddef>
#include <stdexcept>

// Hit-count simulator: stored values must equal their keys.
// Both caches must outlive this object and must not be accessed independently.
template <typename L1Cache, typename L2Cache>
class multilevel_cache_t {
  public:
    multilevel_cache_t(L1Cache &l1, L2Cache &l2) : l1_(l1), l2_(l2) {
        if (l1_.capacity_ >= l2_.capacity_)
            throw std::invalid_argument("L1 capacity must be smaller than L2 capacity");
    }

    template <typename KeyT, typename F>
    bool lookup_update(KeyT key, F slow_get_value) {
        bool l2_accessed = false;
        bool l2_hit = false;

        auto load_from_l2 = [&](KeyT requested_key) {
            l2_accessed = true;
            l2_hit = l2_.lookup_update(requested_key, [&](KeyT memory_key) {
                memory_accesses_++;
                return slow_get_value(memory_key);
            });
            hits_l2_ += l2_hit;

            // lookup_update returns only bool; in this simulator value == key.
            return requested_key;
        };

        bool l1_hit = l1_.lookup_update(key, load_from_l2);
        hits_l1_ += l1_hit;

        // Existing caches do not call their loader when capacity is zero.
        if (!l1_hit && !l2_accessed)
            load_from_l2(key);

        return l1_hit || l2_hit;
    }

    size_t hits_l1_ = 0;
    size_t hits_l2_ = 0;
    size_t memory_accesses_ = 0;

  private:
    L1Cache &l1_;
    L2Cache &l2_;
};
