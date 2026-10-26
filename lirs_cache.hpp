#pragma once

#include "cache_common.hpp"

template <typename T, typename KeyT = size_t> struct lirs_cache_t {

  lirs_cache_t(size_t capacity)
      : capacity_(capacity),
        lhirs_(capacity == 0 ? 0 : std::max<size_t>(1, capacity / 4)),
        llirs_(capacity_ - lhirs_) {}

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {

    if (capacity_ == 0)
      return false;

    auto page_iter = pages_.find(key);

    // new element
    if (page_iter == pages_.end()) {

      T value = slow_get_value(key);

      // firstly filling LIR part of cache
      if (lir_size_ < llirs_) {

        cache_[key] = value;

        stack_.emplace_front(key);
        pages_[key] = {LIR, true, false, stack_.begin(), {}};

        lir_size_++;
        size_++;

        ON_DEBAG(std::cout << "Slowly got new LIR element " << key << "\n";
                 print_cache(););

        return false;
      }

      // all other new elements are resident HIR
      if (queue_.size() == lhirs_)
        evict_hir();

      cache_[key] = value;

      stack_.emplace_front(key);
      queue_.emplace_front(key);

      pages_[key] = {RESIDENT_HIR, true, true, stack_.begin(), queue_.begin()};
      size_++;

      prune_stack();

      ON_DEBAG(std::cout << "Slowly got new resident HIR element " << key
                         << "\n";
               print_cache(););

      return false;
    }

    auto &page = page_iter->second;

    // LIR hit
    if (page.status_ == LIR) {

      stack_.splice(stack_.begin(), stack_, page.stack_iter_);
      page.stack_iter_ = stack_.begin();

      prune_stack();

      ON_DEBAG(std::cout << "Founded LIR element " << key << "\n";
               print_cache(););

      return true;
    }

    // resident HIR hit
    if (page.status_ == RESIDENT_HIR) {

      // HIR is in stack S -> it becomes LIR
      if (page.in_stack_) {

        queue_.erase(page.queue_iter_);
        page.in_queue_ = false;

        page.status_ = LIR;
        lir_size_++;

        stack_.splice(stack_.begin(), stack_, page.stack_iter_);
        page.stack_iter_ = stack_.begin();

        demote_bottom_lir();
        prune_stack();

      } else {

        // resident HIR which is only in Q stays HIR
        stack_.emplace_front(key);
        page.stack_iter_ = stack_.begin();
        page.in_stack_ = true;

        queue_.splice(queue_.begin(), queue_, page.queue_iter_);
        page.queue_iter_ = queue_.begin();
      }

      ON_DEBAG(std::cout << "Founded resident HIR element " << key << "\n";
               print_cache(););

      return true;
    }

    // non-resident HIR hit in stack S: cache miss, but it becomes LIR
    T value = slow_get_value(key);

    if (queue_.size() == lhirs_)
      evict_hir();

    cache_[key] = value;
    page.status_ = LIR;
    lir_size_++;
    size_++;

    stack_.splice(stack_.begin(), stack_, page.stack_iter_);
    page.stack_iter_ = stack_.begin();

    demote_bottom_lir();
    prune_stack();

    ON_DEBAG(std::cout << "Slowly got non-resident HIR element " << key << "\n";
             print_cache(););

    return false;
  }

  void print_cache() {

    std::cout << "\n\nPRINTING LIRS CACHE\n"
              << "cache size is " << size_ << " and capacity is " << capacity_
              << "\n"
              << "LIR size is " << lir_size_ << " and LIR limit is " << llirs_
              << "\n"
              << "HIR queue size is " << queue_.size() << " and Lhirs is "
              << lhirs_ << "\n";

    std::cout << "\nSTACK S:\n";
    for (auto it = stack_.begin(); it != stack_.end(); it++) {
      auto page_iter = pages_.find(*it);

      std::cout << *it << " ";

      if (page_iter->second.status_ == LIR)
        std::cout << "LIR\n";
      else if (page_iter->second.status_ == RESIDENT_HIR)
        std::cout << "resident HIR\n";
      else
        std::cout << "non-resident HIR\n";
    }

    std::cout << "\nQUEUE Q:\n";
    for (auto it = queue_.begin(); it != queue_.end(); it++)
      std::cout << *it << " resident HIR\n";

    std::cout << "\nCACHE:\n";
    for (auto it = cache_.begin(); it != cache_.end(); it++)
      std::cout << it->first << " value " << it->second << "\n";

    std::cout << "----------------------\n\n";
  }

private:
  enum page_status_t { LIR, RESIDENT_HIR, NON_RESIDENT_HIR };

  struct page_t {
    page_status_t status_;

    bool in_stack_ = false;
    bool in_queue_ = false;

    typename std::list<KeyT>::iterator stack_iter_;
    typename std::list<KeyT>::iterator queue_iter_;
  };

  void evict_hir() {

    if (queue_.empty())
      return;

    KeyT key = queue_.back();
    auto page_iter = pages_.find(key);
    auto &page = page_iter->second;

    queue_.pop_back();
    page.in_queue_ = false;

    cache_.erase(key);
    size_--;

    // if HIR is still in S, save it as non-resident HIR
    if (page.in_stack_) {
      page.status_ = NON_RESIDENT_HIR;
      return;
    }

    // HIR which is neither in S nor resident is a new element again
    pages_.erase(page_iter);
  }

  void demote_bottom_lir() {

    if (stack_.empty())
      return;

    KeyT key = stack_.back();
    auto page_iter = pages_.find(key);
    auto &page = page_iter->second;

    if (page.status_ != LIR)
      return;

    page.status_ = RESIDENT_HIR;
    lir_size_--;

    queue_.emplace_front(key);
    page.queue_iter_ = queue_.begin();
    page.in_queue_ = true;
  }

  void prune_stack() {

    // HIR cannot be at the bottom of stack S
    while (!stack_.empty()) {

      KeyT key = stack_.back();
      auto page_iter = pages_.find(key);
      auto &page = page_iter->second;

      if (page.status_ == LIR)
        return;

      stack_.pop_back();
      page.in_stack_ = false;

      // resident HIR remains only in Q
      if (page.status_ == RESIDENT_HIR)
        continue;

      // non-resident HIR outside S and Q becomes completely new
      pages_.erase(page_iter);
    }
  }

  size_t size_ = 0;
  size_t lir_size_ = 0;

  size_t capacity_;
  size_t lhirs_;
  size_t llirs_;

  std::list<KeyT> stack_; // LIRS Stack S
  std::list<KeyT> queue_; // List Q: resident HIR only

  std::unordered_map<KeyT, T> cache_;      // resident elements
  std::unordered_map<KeyT, page_t> pages_; // statuses and iterators
};
