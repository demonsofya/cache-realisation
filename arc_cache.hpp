#pragma once

#include "cache_common.hpp"

template <typename T, typename KeyT = size_t> struct arc_cache_t {
  arc_cache_t(size_t capacity) : capacity_(capacity), p_(0) {}

  size_t capacity_;
  size_t p_;

  std::list<std::pair<KeyT, T>> t1_counter_;
  using T1Iter = typename std::list<std::pair<KeyT, T>>::iterator;
  std::unordered_map<KeyT, T1Iter> t1_cache_;

  std::list<std::pair<KeyT, T>> t2_counter_;
  using T2Iter = typename std::list<std::pair<KeyT, T>>::iterator;
  std::unordered_map<KeyT, T2Iter> t2_cache_;

  std::list<KeyT> b1_counter_; // evicted from t1
  using B1Iter = typename std::list<KeyT>::iterator;
  std::unordered_map<KeyT, B1Iter>
      b1_cache_; // if found here t1 should be increased

  std::list<KeyT> b2_counter_; // evicted from t2
  using B2Iter = typename std::list<KeyT>::iterator;
  std::unordered_map<KeyT, B2Iter>
      b2_cache_; // if found here t2 should be increased

  void replace(KeyT key) {
    if (!t1_counter_.empty() &&
        (t1_counter_.size() > p_ || (b2_cache_.find(key) != b2_cache_.end() &&
                                     t1_counter_.size() == p_))) {

      KeyT old_key = t1_counter_.back().first;

      t1_cache_.erase(old_key);
      t1_counter_.pop_back();

      b1_counter_.emplace_front(old_key);
      b1_cache_[old_key] = b1_counter_.begin();

    } else if (!t2_counter_.empty()) {

      KeyT old_key = t2_counter_.back().first;

      t2_cache_.erase(old_key);
      t2_counter_.pop_back();

      b2_counter_.emplace_front(old_key);
      b2_cache_[old_key] = b2_counter_.begin();
    }
  }

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {
    if (!capacity_)
      return false;

    auto t1_iter = t1_cache_.find(key);

    if (t1_iter != t1_cache_.end()) { // elem in t1 -> move it to the front of
                                      // t2
      T value = t1_iter->second->second;

      t1_counter_.erase(t1_iter->second);
      t1_cache_.erase(t1_iter);

      t2_counter_.emplace_front(key, value);
      t2_cache_[key] = t2_counter_.begin();

      return true;
    }

    auto t2_iter = t2_cache_.find(key);

    if (t2_iter !=
        t2_cache_.end()) { // elem in t2 already - just move it the front of t2
      t2_counter_.splice(t2_counter_.begin(), t2_counter_, t2_iter->second);
      return true;
    }

    auto b1_iter = b1_cache_.find(key);

    if (b1_iter != b1_cache_.end()) {
      size_t delta =
          std::max<size_t>(1, b2_counter_.size() / b1_counter_.size());

      p_ = std::min(capacity_, p_ + delta);

      replace(key);

      b1_counter_.erase(b1_iter->second);
      b1_cache_.erase(b1_iter);

      T value = slow_get_value(key);

      t2_counter_.emplace_front(key, value);
      t2_cache_[key] = t2_counter_.begin();

      return false;
    }

    auto b2_iter = b2_cache_.find(key);

    if (b2_iter != b2_cache_.end()) {
      size_t delta =
          std::max<size_t>(1, b1_counter_.size() / b2_counter_.size());

      p_ = (delta > p_) ? 0 : p_ - delta;

      replace(key);

      b2_counter_.erase(b2_iter->second);
      b2_cache_.erase(b2_iter);

      T value = slow_get_value(key);

      t2_counter_.emplace_front(key, value);
      t2_cache_[key] = t2_counter_.begin();

      return false;
    }
    // last case - new element
    size_t l1_size = t1_counter_.size() + b1_counter_.size();
    size_t whole_size = l1_size + t2_counter_.size() + b2_counter_.size();

    if (l1_size == capacity_) {
      if (t1_counter_.size() < capacity_) {
        if (!b1_counter_.empty()) { // if b1 has elements the last one should be
                                    // evicted
          KeyT old_key = b1_counter_.back();

          b1_cache_.erase(old_key);
          b1_counter_.pop_back();
        }

        replace(key);
      }

      else { // if b1 is empty only t1 should be changed
        KeyT old_key = t1_counter_.back().first;

        t1_cache_.erase(old_key);
        t1_counter_.pop_back();
      }
    }

    else { // l1_size < capacity_ so there's also t2 and b2
      if (whole_size >= capacity_) {
        if (whole_size == 2 * capacity_ &&
            !b2_counter_
                 .empty()) { // b1 and b2 can't take up mpre than capacity_
          KeyT old_key = b2_counter_.back();

          b2_cache_.erase(old_key);
          b2_counter_.pop_back();
        }

        replace(key);
      }
    }
    T value = slow_get_value(key);

    t1_counter_.emplace_front(key, value);
    t1_cache_[key] = t1_counter_.begin();

    return false;
  }

  void print_cache() {
    std::cout << "\n\nPRINTING CACHE\n"
              << "capacity is " << capacity_ << ", p is " << p_ << "\n";

    std::cout << "\nPRINTING T1_\n";
    for (auto it = t1_counter_.begin(); it != t1_counter_.end(); it++) {
      std::cout << it->first << " value " << it->second << "\n";
    }

    std::cout << "\nPRINTING T2_\n";
    for (auto it = t2_counter_.begin(); it != t2_counter_.end(); it++) {
      std::cout << it->first << " value " << it->second << "\n";
    }

    std::cout << "\nPRINTING B1_\n";
    for (auto it = b1_counter_.begin(); it != b1_counter_.end(); it++) {
      std::cout << *it << "\n";
    }

    std::cout << "\nPRINTING B2_\n";
    for (auto it = b2_counter_.begin(); it != b2_counter_.end(); it++) {
      std::cout << *it << "\n";
    }

    std::cout << "----------------------\n\n";
  }
};
