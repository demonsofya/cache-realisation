#pragma once

#include "cache_common.hpp"

template <typename T, typename KeyT = size_t>
struct lfu_cache_t {

    lfu_cache_t(size_t capacity) : capacity_(capacity) {}

    // ~lfu_cache_t() = default;

    template <typename F, typename OnEvict = ignore_eviction_t>
    bool lookup_update(KeyT key, F slow_get_value, OnEvict on_evict = {}) {

        if (capacity_ == 0) {
            return false;
        }

        auto value_iter = cache_.find(key);
        if (value_iter != cache_.end()) {

            auto curr_it = counter_.begin();
            for (; curr_it != counter_.end(); curr_it++) {
                if (curr_it->first == key)
                    break;
            }

            ++(curr_it->second);

            auto next_it = std::next(curr_it);
            while (next_it != counter_.end() && next_it->second < curr_it->second)
                next_it++;

            counter_.splice(next_it, counter_, curr_it);

            ON_DEBAG(std::cout << "Founded element " << curr_it->first; print_cache(););

            // if (next_it != counter_.end()) {

            //   ON_DEBAG(std::cout << "Founded element " << curr_it->first
            //                      << "and swaped it with " << next_it->first <<
            //                      "\n";
            //            print_cache(););

            // } else {

            //   ON_DEBAG(std::cout << "Founded element " << curr_it->first
            //                      << " and didnt swaped\n");
            // }

            return true;
        }

        T value = slow_get_value(key);
        if (size_ < capacity_) {

            cache_[key] = value;
            counter_.emplace_front(std::make_pair(key, static_cast<size_t>(1)));

            size_++;

            ON_DEBAG(std::cout << "Slowly got elem " << value << "\n"; print_cache(););

            return false;
        }

        auto deliting_elem = counter_.begin();
        on_evict(deliting_elem->first);
        cache_.erase(deliting_elem->first);

        *deliting_elem = {key, static_cast<size_t>(1)}; // canging counter_
        cache_[key] = value;                            // changing cache_

        ON_DEBAG(std::cout << "Slowly got elem " << value << "\n"; print_cache(););

        return false;
    }

    size_t capacity() const { return capacity_; }

    bool contains(KeyT key) const { return cache_.contains(key); }

    bool erase(KeyT key) {
        if (!cache_.erase(key))
            return false;
        auto it = std::find_if(counter_.begin(), counter_.end(),
                               [&](const auto &entry) { return entry.first == key; });
        counter_.erase(it);
        size_--;
        return true;
    }

    std::vector<KeyT> resident_keys() const {
        std::vector<KeyT> keys;
        for (const auto &entry : cache_)
            keys.push_back(entry.first);
        return keys;
    }

    void print_cache() {

        std::cout << "\n\nPRINTING COUNTER_\n"
                  << "cache size is " << size_ << " and capacity is " << capacity_ << "\n";

        for (auto it = counter_.begin(); it != counter_.end(); it++) {
            std::cout << it->first << " freq " << it->second << "\n";
        }

        std::cout << "\nPRINTING CACHE_\n";
        for (auto it = cache_.begin(); it != cache_.end(); it++) {
            std::cout << it->second << "\n";
        }

        std::cout << "----------------------\n\n";
    }

    size_t size_ = 0;
    size_t capacity_;

    std::unordered_map<KeyT, T> cache_;
    std::list<std::pair<KeyT, size_t>> counter_;
};
