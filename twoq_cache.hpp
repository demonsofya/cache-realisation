#pragma once

#include "lru_cache.hpp"

template <typename T, typename KeyT = size_t> struct twoq_cache_t {
  twoq_cache_t(size_t capacity)
      : capacity_(capacity),
        kin_(capacity == 0
                 ? 0
                 : std::max<size_t>(1, capacity / 4)), // kin ~ 25% capacity
        kout_(capacity == 0
                  ? 0
                  : std::max<size_t>(1, capacity / 2)), // kout ~ 50% capacity
        am_cache_(capacity) {}

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {
    if (capacity_ == 0)
      return false;

    auto am_iter = am_cache_.cache_.find(key);
    if (am_iter != am_cache_.cache_.end()) {
      am_cache_.lookup_update(key,
                              slow_get_value); // if in am - just move forward
      return true;
    }

    auto a1in_iter = a1in_cache_.find(key);
    if (a1in_iter != a1in_cache_.end())
      return true; // if in a1in - do nothing

    auto a1out_iter = a1out_cache_.find(key);
    if (a1out_iter != a1out_cache_.end()) { // if in a1out - move to am
      a1out_counter_.erase(a1out_iter->second);
      a1out_cache_.erase(a1out_iter);

      reclaim();

      am_cache_.lookup_update(key, slow_get_value);
      size_++;
      return false;
    }

    T value = slow_get_value(key);

    reclaim();

    a1in_counter_.emplace_front(std::make_pair(key, value));
    a1in_cache_[key] = a1in_counter_.begin();

    size_++;

    ON_DEBAG(std::cout << "Slowly got new elem " << value << "\n";
             print_cache(););

    return false;
  }

  void reclaim() {
    if (size_ < capacity_)
      return;

    if ((!a1in_counter_.empty() && a1in_counter_.size() > kin_) ||
        am_cache_.counter_.empty()) {

      KeyT old_key = a1in_counter_.back().first;

      a1in_cache_.erase(old_key);
      a1in_counter_.pop_back();

      a1out_counter_.emplace_front(old_key);
      a1out_cache_[old_key] = a1out_counter_.begin();

      if (a1out_counter_.size() > kout_) {
        KeyT evicted_key = a1out_counter_.back();

        a1out_cache_.erase(evicted_key);
        a1out_counter_.pop_back();
      }

    } else {
      if (!(am_cache_.counter_.empty())) {
        am_cache_.cache_.erase(am_cache_.counter_.back().first);
        am_cache_.counter_.pop_back();
        am_cache_.size_--;
      }
    }

    size_--;
  }

  void print_cache() {

    std::cout << "\n\nPRINTING COUNTER_\n"
              << "cache size is " << size_ << " and capacity is " << capacity_
              << "\n";

    std::cout << "\nPRINTING A1IN_CACHE_\n";
    for (auto it = a1in_cache_.begin(); it != a1in_cache_.end(); it++) {
      std::cout << it->second->first << " value " << it->second->second << "\n";
    }

    std::cout << "\nPRINTING A1OUT_CACHE_\n";
    for (auto it = a1out_counter_.begin(); it != a1out_counter_.end(); it++) {
      std::cout << *it << "\n";
    }

    std::cout << "\nPRINTING AM_CACHE_\n";
    for (auto it = am_cache_.counter_.begin(); it != am_cache_.counter_.end();
         it++) {
      std::cout << it->first << " value " << it->second << "\n";
    }

    std::cout << "----------------------\n\n";
  }

  size_t size_ = 0;
  size_t capacity_;

  size_t kin_;  // max A1in size
  size_t kout_; // max A1out size

  std::list<std::pair<KeyT, T>> a1in_counter_;

  using A1inIter = typename std::list<std::pair<KeyT, T>>::iterator;
  std::unordered_map<KeyT, A1inIter> a1in_cache_; // firstly met elems

  std::list<KeyT> a1out_counter_;

  using A1outIter = typename std::list<KeyT>::iterator;
  std::unordered_map<KeyT, A1outIter> a1out_cache_; // evicted recently from
                                                    // a1in

  lru_cache_t<T, KeyT> am_cache_; // found in a1out = met multiple times
};
