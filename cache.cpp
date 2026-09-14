#include <cctype>
#include <cstddef>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "cache.hpp"

using KeyT = size_t;

size_t slow_get_value(KeyT key) {

  return key; // cause its not really important how is testing works
}

int main() {

  size_t cache_sz = 0, data_count = 0;
  std::cin >> cache_sz >> data_count;

  lfu_cache_t<size_t> cache_lfu(cache_sz);
  lru_cache_t<size_t> cache_lru(cache_sz);

  size_t hits_lfu = 0, hits_lru = 0;
  for (int i = 0; i < data_count; i++) {
    KeyT key;
    std::cin >> key;

    if (cache_lfu.lookup_update(key, slow_get_value))
      hits_lfu++;

    if (cache_lru.lookup_update(key, slow_get_value))
      hits_lru++;
  }

  std::cout << "LFU hits: " << hits_lfu << "\nLRU hits: " << hits_lru << "\n";
  return 0;
}