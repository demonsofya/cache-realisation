#pragma once

#include "cache_common.hpp"

template <typename T, typename KeyT = size_t> struct lru_cache_t {

  lru_cache_t(size_t capacity) : capacity_(capacity) {}

  // ~lru_cache_t() = default;

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {

    if (capacity_ == 0)
      return false;

    auto cache_iter = cache_.find(key);
    if (cache_iter != cache_.end()) {

      auto counter_iter = cache_iter->second;
      counter_.splice(counter_.begin(), counter_, counter_iter);

      ON_DEBAG(std::cout << "Found elem " << counter_iter->first << "\n";
               print_cache(););

      return true;
    }

    T value = slow_get_value(key);
    if (size_ == capacity_) {

      cache_.erase(counter_.back().first);

      ON_DEBAG(std::cout << "Deleting from counter and cache elemet "
                         << counter_.begin()->first << "\n");

      counter_.pop_back(); // last recently used element

    } else {

      size_++;
    }

    counter_.emplace_front(std::make_pair(key, value));
    cache_[key] = counter_.begin();

    ON_DEBAG(std::cout << "Slowly got elem " << value << "\n"; print_cache(););

    return false;
  }

  void print_cache() {

    std::cout << "\n\nPRINTING COUNTER_\n"
              << "cache size is " << size_ << " and capacity is " << capacity_
              << "\n";

    for (auto it = counter_.begin(); it != counter_.end(); it++) {
      std::cout << it->first << " value " << it->second << "\n";
    }

    std::cout << "\nPRINTING CACHE_\n";
    for (auto it = cache_.begin(); it != cache_.end(); it++) {
      std::cout << it->second->first << " value " << it->second->second << "\n";
    }

    std::cout << "----------------------\n\n";
  }

  size_t size_ = 0;
  size_t capacity_;

  std::list<std::pair<KeyT, T>> counter_;
  using CounterIter = typename std::list<std::pair<KeyT, T>>::iterator;

  std::unordered_map<KeyT, CounterIter> cache_;
};
