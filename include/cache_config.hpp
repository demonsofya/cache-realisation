#pragma once

#include "cache_hierarchy.hpp"

#include <string>
#include <vector>

std::vector<cache_level_config_t> make_levels(
    const std::vector<std::string> &policies, size_t capacity,
    const std::string &distribution = "geometric", size_t ratio = 2,
    const std::vector<size_t> &sizes = {});
