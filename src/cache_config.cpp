#include "cache_config.hpp"

#include <limits>
#include <stdexcept>

namespace {
size_t checked_add(size_t a, size_t b) {
    if (b > std::numeric_limits<size_t>::max() - a)
        throw std::overflow_error("Capacity or weight sum overflow");
    return a + b;
}

size_t checked_multiply(size_t a, size_t b) {
    if (b && a > std::numeric_limits<size_t>::max() / b)
        throw std::overflow_error("Capacity or weight product overflow");
    return a * b;
}
} // namespace

std::vector<cache_level_config_t> make_levels(
    const std::vector<std::string> &policies, size_t capacity,
    const std::string &distribution, size_t ratio, const std::vector<size_t> &sizes) {
    if (policies.empty())
        throw std::invalid_argument("At least one level is required");
    if (distribution != "equal" && distribution != "linear" && distribution != "geometric")
        throw std::invalid_argument("Unknown capacity distribution");
    if (distribution == "geometric" && ratio < 2)
        throw std::invalid_argument("Geometric ratio must be at least 2");

    std::vector<size_t> capacities = sizes;
    if (!sizes.empty()) {
        if (sizes.size() != policies.size())
            throw std::invalid_argument("--sizes must contain one capacity per level");
        size_t sum = 0;
        for (size_t size : sizes)
            sum = checked_add(sum, size);
        if (sum != capacity)
            throw std::invalid_argument("--sizes must sum to the input capacity");
    } else {
        std::vector<size_t> weights;
        size_t sum = 0, geometric_weight = 1;
        for (size_t i = 0; i < policies.size(); i++) {
            size_t weight = distribution == "equal" ? 1 : i + 1;
            if (distribution == "geometric") {
                if (i)
                    geometric_weight = checked_multiply(geometric_weight, ratio);
                weight = geometric_weight;
            }
            weights.push_back(weight);
            sum = checked_add(sum, weight);
        }
        size_t remaining = capacity;
        for (size_t i = 0; i < weights.size(); i++) {
            // Last level receives the rounding remainder; no slots are lost.
            size_t size = remaining;
            if (i + 1 != weights.size())
                size = (capacity / sum) * weights[i]
                     + checked_multiply(capacity % sum, weights[i]) / sum;
            capacities.push_back(size);
            remaining -= size;
        }
    }

    std::vector<cache_level_config_t> levels;
    for (size_t i = 0; i < policies.size(); i++) {
        if (policies.size() > 1 && capacities[i] == 0)
            throw std::invalid_argument("Allocation gives an empty level; increase capacity or use --sizes");
        levels.push_back({policies[i], capacities[i]});
    }
    return levels;
}
