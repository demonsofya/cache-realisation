#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <unordered_map>
#include <utility>

// #define DEBUG

#ifdef DEBUG
#define ON_DEBAG(...) __VA_ARGS__
#else
#define ON_DEBAG(...)
#endif
