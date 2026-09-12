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

    lfu_cache_t<size_t> cache(cache_sz);

    size_t hits = 0;
    for (int i = 0; i < data_count; i++) {
        KeyT key;
        std::cin >> key;

        if (cache.lookup_update(key, slow_get_value))
            hits++;
    }

    std::cout << hits << "\n";
    return 0;
}