#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <utility>

template <typename T, typename KeyT = size_t> struct lfu_cache_t {

  lfu_cache_t(size_t sz) : capacity_(sz) {}

  ~lfu_cache_t() = default;

  template <typename F> bool lookup_update(KeyT key, F slow_get_value) {
    if (capacity_ == 0) {
      return false;
    }

    auto value_iter = cahce_.find(key);
    if (value_iter != cahce_.end()) {

      auto curr_it = counter_.begin();
      for (; curr_it != counter_.back(); curr_it++) {
        if (curr_it->fist == key)
          break;
      }

      size_t freq = ++(curr_it->second);

      auto next_it = curr_it + 1;
      while (next_it->second < freq && next_it != counter_.back())
        next_it++;

      std::swap(curr_it, next_it); // сортируем так чтобы было как надо, просто свапаем тк разница максимум 1
      return true; 
    }

    T value = slow_get_value(key);

    if (size_ < capacity_) {
      cahce_[key] = value;              // сразу фигачим элемент в кеш
      counter_.emplace_front({key, 1}); // добавяем счетчик одного обращения

      return false;
    }

    auto deliting_elem = counter_.begin();
    *deliting_elem = {key, value};
    
    return false;
  }

  size_t size_; // текущий размер всего
  size_t capacity_;
  std::unordered_map<KeyT, T> cahce_; // похуй на сортировку
  std::list<std::pair<KeyT, size_t>>
      counter_; // сортируем по ключам чтобы в начале лежал тот, который хотим
                // удалить
  // эта штука хранит количество обращений по ключам - они по ключам
  // отсортированны
};