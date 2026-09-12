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

  lfu_cache_t(size_t sz) : capacity_(sz) {}

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

      size_t freq = ++(curr_it->second);

      auto next_it = curr_it;
      next_it++;

      while (next_it->second < freq && next_it != counter_.end())
        next_it++;

      if (next_it != counter_.end()) {
        ON_DEBAG(std::cout << "Founded element " << curr_it->first
                           << "and swaped it with " << next_it->first << "\n";
                 print_cache(););
        std::swap(curr_it, next_it); // сортируем так чтобы было как надо,
                                     // просто свапаем тк разница максимум 1
      } else {
        ON_DEBAG(std::cout << "Founded element " << curr_it->first
                           << " and didnt swaped\n");
      }
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
    cache_[key] = value; // changing cache_

    ON_DEBAG(std::cout << "Slowly got elem " << value << "\n"; print_cache(););
    return false;
  }

  void print_cache() {
    std::cout << "\n\nPRINTING COUNTER_\n";
    std::cout << "cache size is " << size_ << " and capacity is " << capacity_
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