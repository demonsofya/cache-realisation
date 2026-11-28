#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

// #define DEBUG

#ifdef DEBUG
#define ON_DEBAG(...) __VA_ARGS__
#else
#define ON_DEBAG(...)
#endif

// Receives a resident key when insertion evicts it.
struct ignore_eviction_t {
    template <typename KeyT>
    void operator()(const KeyT &) const {}
};
