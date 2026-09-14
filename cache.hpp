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

      //   std::swap(curr_it, next_it); // сортируем так чтобы было как надо,
      //                                // просто свапаем тк разница максимум 1
      // } else {

      //   ON_DEBAG(std::cout << "Founded element " << curr_it->first
      //                      << " and didnt swaped\n");
      // }

      return true;
    }

    T value = slow_get_value(key);
    if (size_ < capacity_) {

      cache_[key] = value; // сразу фигачим элемент в кеш
      counter_.emplace_front(std::make_pair(
          key, static_cast<size_t>(1))); // добавяем счетчик одного обращения

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

  size_t size_ = 0; // текущий размер всего
  size_t capacity_;

  std::unordered_map<KeyT, T> cache_; // похуй на сортировку
  std::list<std::pair<KeyT, size_t>>
      counter_; // сортируем по ключам чтобы в начале лежал тот, который хотим
                // удалить

  // эта штука хранит количество обращений по ключам - они по ключам
  // отсортированны
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

      ON_DEBAG(std::cout << "Founeded elem " << counter_iter->first << "\n";
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

  size_t size_ = 0; // текущий размер всего
  size_t capacity_;

  std::list<std::pair<KeyT, T>> counter_; // сортируем чтобы в начале лежал
  // тот, который хотим удалить
  using CounterIter = typename std::list<std::pair<KeyT, T>>::iterator;

  std::unordered_map<KeyT, CounterIter> cache_; // похуй на сортировку
};