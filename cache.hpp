#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <sys/types.h>
#include <unordered_map>
#include <utility>

#define DEBUG

#ifdef DEBUG
#define ON_DEBAG(...) __VA_ARGS__
#else
#define ON_DEBAG(...)
#endif

template <typename T, typename KeyT = size_t> struct lfu_cache_t {

  lfu_cache_t(size_t capacity) : capacity_(capacity) {}

  // ~lfu_cache_t() = default;

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {

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

      ON_DEBAG(std::cout << "Founded element " << curr_it->first;
               print_cache(););

      // if (next_it != counter_.end()) {

      //   ON_DEBAG(std::cout << "Founded element " << curr_it->first
      //                      << "and swaped it with " << next_it->first <<
      //                      "\n";
      //            print_cache(););

      //   std::swap(curr_it, next_it); // ��������� ��� ����� ���� ��� ����,
      //                                // ������ ������� �� ������� �������� 1
      // } else {

      //   ON_DEBAG(std::cout << "Founded element " << curr_it->first
      //                      << " and didnt swaped\n");
      // }

      return true;
    }

    T value = slow_get_value(key);
    if (size_ < capacity_) {

      cache_[key] = value; // ����� ������� ������� � ���
      counter_.emplace_front(std::make_pair(
          key, static_cast<size_t>(1))); // �������� ������� ������ ���������

      size_++;

      ON_DEBAG(std::cout << "Slowly got elem " << value << "\n";
               print_cache(););

      return false;
    }

    auto deliting_elem = counter_.begin();
    cache_.erase(deliting_elem->first);

    *deliting_elem = {key, static_cast<size_t>(1)}; // canging counter_
    cache_[key] = value;                            // changing cache_

    ON_DEBAG(std::cout << "Slowly got elem " << value << "\n"; print_cache(););

    return false;
  }

  void print_cache() {

    std::cout << "\n\nPRINTING COUNTER_\n"
              << "cache size is " << size_ << " and capacity is " << capacity_
              << "\n";

    for (auto it = counter_.begin(); it != counter_.end(); it++) {
      std::cout << it->first << " freq " << it->second << "\n";
    }

    std::cout << "\nPRINTING CACHE_\n";
    for (auto it = cache_.begin(); it != cache_.end(); it++) {
      std::cout << it->second << "\n";
    }

    std::cout << "----------------------\n\n";
  }

  size_t size_ = 0; // ������� ������ �����
  size_t capacity_;

  std::unordered_map<KeyT, T> cache_; // ����� �� ����������
  std::list<std::pair<KeyT, size_t>>
      counter_; // ��������� �� ������ ����� � ������ ����� ���, ������� �����
                // �������

  // ��� ����� ������ ���������� ��������� �� ������ - ��� �� ������
  // ��������������
};

//==================================================================

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

  size_t size_ = 0; // ������� ������ �����
  size_t capacity_;

  std::list<std::pair<KeyT, T>> counter_; // ��������� ����� � ������ �����
  // ���, ������� ����� �������
  using CounterIter = typename std::list<std::pair<KeyT, T>>::iterator;

  std::unordered_map<KeyT, CounterIter> cache_; // ����� �� ����������
};


template <typename T, typename KeyT = size_t> struct twoq_cache_t {
  twoq_cache_t(size_t capacity) : capacity_(capacity), 
                                  am_cache_(capacity),
                                  kin_(capacity == 0 ? 0 : std::max<size_t>(1, capacity / 4)), // kin ~ 25% capacity
                                  kout_(capacity == 0 ? 0 : std::max<size_t>(1, capacity / 2)) {} //kout ~ 50% capacity

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {
    if (capacity_ == 0) return false; 

    auto am_iter = am_cache_.cache_.find(key);
    if (am_iter != am_cache_.cache_.end()) {
      am_cache_.lookup_update(key, slow_get_value);   //if in am - just move forward
      return true;
    }

    auto a1in_iter = a1in_cache_.find(key);
    if (a1in_iter != a1in_cache_.end()) return true;  //if in a1in - do nothing

    auto a1out_iter = a1out_cache_.find(key);
    if (a1out_iter != a1out_cache_.end()) {           //if in a1out - move to am
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
    if (size_ < capacity_) return;

    if (!a1in_counter_.empty() && a1in_counter_.size() > kin_ || am_cache_.counter_.empty()) {

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
    for (auto it = am_cache_.counter_.begin();
      it != am_cache_.counter_.end(); it++) {
      std::cout << it->first << " value " << it->second << "\n";
    }

    std::cout << "----------------------\n\n";
  }

  size_t size_ = 0;
  size_t capacity_;

  size_t kin_;  //max A1in size
  size_t kout_; //max A1out size

  std::list<std::pair<KeyT, T>> a1in_counter_;

  using A1inIter = typename std::list<std::pair<KeyT, T>>::iterator;
  std::unordered_map<KeyT, A1inIter> a1in_cache_; //firstly met elems

  std::list<KeyT> a1out_counter_;

  using A1outIter = typename std::list<KeyT>::iterator;
  std::unordered_map<KeyT, A1outIter> a1out_cache_;  //evicted recently from a1in

  lru_cache_t<T, KeyT> am_cache_;  //found in a1out = met multiple times
};



template <typename T, typename KeyT = size_t> struct arc_cache_t {
  arc_cache_t(size_t capacity) : capacity_(capacity), p_(0) {}

  size_t capacity_;

  std::list<std::pair<KeyT, T>> t1_counter_;
  using T1Iter = typename std::list<std::pair<KeyT, T>>::iterator;
  std::unordered_map<KeyT, T1Iter> t1_cache_;

  std::list<std::pair<KeyT, T>> t2_counter_;
  using T2Iter = typename std::list<std::pair<KeyT, T>>::iterator;
  std::unordered_map<KeyT, T2Iter> t2_cache_;

  std::list<KeyT> b1_counter_; //evicted from t1
  using B1Iter = typename std::list<KeyT>::iterator; 
  std::unordered_map<KeyT, B1Iter> b1_cache_; //if found here t1 should be increased

  std::list<KeyT> b2_counter_; //evicted from t2
  using B2Iter = typename std::list<KeyT>::iterator;
  std::unordered_map<KeyT, B2Iter> b2_cache_; //if found here t2 should be increased

  void replace(KeyT key) {
    if (!t1_counter_.empty() &&
        (t1_counter_.size() > p_ || 
        (b2_cache_.find(key) != b2_cache_.end() && t1_counter_.size() == p_))) {

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
    if (!capacity_) return false;

    auto t1_iter = t1_cache_.find(key);

    if (t1_iter != t1_cache_.end()) { //elem in t1 -> move it to the front of t2
      T value = t1_iter->second->second;

      t1_counter_.erase(t1_iter->second);
      t1_cache_.erase(t1_iter);

      t2_counter_.emplace_front(key, value);
      t2_cache_[key] = t2_counter_.begin();

      return true;
    }

    auto t2_iter = t2_cache_.find(key);

    if (t2_iter != t2_cache_.end()) { //elem in t2 already - just move it the front of t2
      t2_counter_.splice(t2_counter_.begin(), t2_counter_, t2_iter->second);
      return true;
    }

    auto b1_iter = b1_cache_.find(key);

    if (b1_iter != b1_cache_.end()) { //elem in b1 = it has been kicked from t1 -> increase p_
      size_t max_val = std::max<size_t> (1, b2_counter_.size()/b1_counter_.size());
      size_t delta = (b1_counter_.empty() ? 1 : max_val);
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
      size_t max_val = std::max<size_t> (1, b1_counter_.size()/b2_counter_.size());
      size_t delta = (b1_counter_.empty() ? 1 : max_val);
      p_ = (delta > p_) ? 0 : p_ - delta;

      replace(key);

      b2_counter_.erase(b2_iter->second);
      b2_cache_.erase(b2_iter);

      T value = slow_get_value(key);

      t2_counter_.emplace_front(key, value);
      t2_cache_[key] = t2_counter_.begin();

      return false;
    }
                //last case - new element
    size_t l1_size = t1_counter_.size() + b1_counter_.size();
    size_t whole_size = l1_size + t2_counter_.size() + b2_counter_.size();

    if (l1_size == capacity_) {
      if (t1_counter_.size() < capacity_) {
        if (!b1_counter_.empty()) { //if b1 has elements the last one should be evicted
          KeyT old_key = b1_counter_.back();

          b1_cache_.erase(old_key);
          b1_counter_.pop_back();
        }

        replace(key);      
      }

      else { //if b1 is empty only t1 should be changed 
        KeyT old_key = t1_counter_.back().first;

        t1_cache_.erase(old_key);
        t1_counter_.pop_back();
      }
    }

    else { //l1_size < capacity_ so there's also t2 and b2
      if (whole_size >= capacity_) {
        if (whole_size == 2 * capacity_ && !b2_counter_.empty()) { //b1 and b2 can't take up mpre than capacity_
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
              << "capacity is " << capacity_
              << ", p is " << p_
              << "\n";

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