// FIXME: what is the purpose of this file?

// FIXME: some of the included headers are not used
#include <cctype>
#include <cstddef>
#include <iostream>

#include "cache.hpp"

#define RETURN_IF_CIN_ERROR                                                                        \
    if (std::cin.fail()) {                                                                         \
        std::cout << "Wrong input\n. Programm ended\n";                                            \
        return 0;                                                                                  \
    }

using KeyT = size_t;
size_t slow_get_value(KeyT key) {

    return key; // cause its not really important how does testing work
}

// FIXME: add function try catch block on main
int main() {
    size_t cache_sz = 0, data_count = 0;
    if (!(std::cin >> cache_sz >> data_count))
        return 1;

    std::vector<KeyT> data(data_count);
    for (auto &key : data) {
        if (!(std::cin >> key))
            return 1;
    }
    // FIXME: check std::cin.fail()

    size_t hits_ideal = ideal_cache_hits(data, cache_sz);

    lfu_cache_t<size_t> cache_lfu(cache_sz);
    lru_cache_t<size_t> cache_lru(cache_sz);
    twoq_cache_t<size_t> cache_twoq(cache_sz);
    arc_cache_t<size_t> cache_arc(cache_sz);
    lirs_cache_t<size_t> cache_lirs(cache_sz);

    size_t hits_lfu = 0, hits_lru = 0;
    size_t hits_twoq = 0, hits_arc = 0, hits_lirs = 0;

    for (auto key : data) {
        if (cache_lfu.lookup_update(key, slow_get_value))
            hits_lfu++;

        if (cache_lru.lookup_update(key, slow_get_value))
            hits_lru++;

        if (cache_twoq.lookup_update(key, slow_get_value))
            hits_twoq++;

        if (cache_arc.lookup_update(key, slow_get_value))
            hits_arc++;

        if (cache_lirs.lookup_update(key, slow_get_value))
            hits_lirs++;
    }

    std::cout << "LFU hits: " << hits_lfu << "\n"
              << "LRU hits: " << hits_lru << "\n"
              << "2Q hits: " << hits_twoq << "\n"
              << "ARC hits: " << hits_arc << "\n"
              << "LIRS hits: " << hits_lirs << "\n"
              << "IDEAL hits: " << hits_ideal << "\n"
              << "IDEAL misses: " << data_count - hits_ideal << "\n";

    return 0;
}