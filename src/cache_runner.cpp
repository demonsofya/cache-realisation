#include "cache_hierarchy.hpp"

#include <charconv>
#include <fstream>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

size_t parse_number(const std::string &token) {
    size_t value = 0;
    auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || end != token.data() + token.size())
        throw std::invalid_argument("Invalid nonnegative integer: " + token);
    return value;
}

std::ifstream open_file(const std::string &path) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Cannot open file: " + path);
    return input;
}

bool next_token(std::istream &input, std::string &token) {
    while (input >> token) {
        if (token.front() != '#')
            return true;
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (input.bad())
        throw std::runtime_error("Input read error");
    return false;
}

size_t read_number(std::istream &input, const char *name) {
    std::string token;
    if (!next_token(input, token))
        throw std::runtime_error(std::string("Missing ") + name);
    return parse_number(token);
}

std::vector<std::string> read_config(const std::string &path) {
    auto input = open_file(path);
    const size_t count = read_number(input, "level count");
    if (count == 0)
        throw std::invalid_argument("At least one cache level is required");

    std::vector<std::string> policies;
    std::string policy;
    for (size_t i = 0; i < count; i++) {
        if (!next_token(input, policy))
            throw std::runtime_error("Fewer policies than the declared level count");
        if (policy != "LRU" && policy != "LFU" && policy != "2Q"
            && policy != "ARC" && policy != "LIRS")
            throw std::invalid_argument("Unknown cache policy: " + policy);
        policies.push_back(policy);
    }
    if (next_token(input, policy))
        throw std::runtime_error("Extra tokens in config");
    return policies;
}

std::vector<cache_level_config_t> make_levels(const std::vector<std::string> &policies,
                                             size_t capacity) {
    const size_t maximum = std::numeric_limits<size_t>::max();
    size_t weight_sum = 0;
    for (size_t i = 0; i < policies.size(); i++) {
        if (maximum - weight_sum < i + 1)
            throw std::overflow_error("Too many levels");
        weight_sum += i + 1;
    }

    std::vector<cache_level_config_t> levels;
    size_t remaining = capacity;
    for (size_t i = 0; i < policies.size(); i++) {
        size_t level_capacity = remaining;
        if (i + 1 != policies.size()) {
            const size_t remainder = capacity % weight_sum;
            if (remainder > maximum / (i + 1))
                throw std::overflow_error("Capacity distribution overflow");
            level_capacity = (capacity / weight_sum) * (i + 1)
                           + remainder * (i + 1) / weight_sum;
        }
        levels.push_back({policies[i], level_capacity});
        remaining -= level_capacity;
    }
    return levels;
}

void run_requests(std::istream &input, cache_hierarchy_t &cache, size_t count) {
    for (size_t i = 0; i < count; i++)
        cache.lookup_update(read_number(input, "request key"));
    std::string extra;
    if (next_token(input, extra))
        throw std::runtime_error("More requests than the declared request count");
}

void print_results(const cache_hierarchy_t &cache, bool json) {
    const size_t hits = cache.requests() - cache.memory_accesses();
    if (json) {
        std::cout << "{\"requests\":" << cache.requests() << ",\"hits\":" << hits
                  << ",\"misses\":" << cache.memory_accesses() << ",\"levels\":[";
        for (size_t i = 0; i < cache.levels().size(); i++) {
            const auto &level = cache.levels()[i];
            if (i != 0)
                std::cout << ',';
            std::cout << "{\"policy\":\"" << level.config.policy << "\",\"capacity\":"
                      << level.config.capacity << ",\"hits\":" << level.hits
                      << ",\"misses\":" << level.misses << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << hits << '\n';
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: cache_runner CONFIG [REQUESTS] [--json]\n";
        return 1;
    }
    try {
        bool json = false;
        std::string request_path;
        for (int i = 2; i < argc; i++) {
            const std::string argument = argv[i];
            if (argument == "--json" && !json)
                json = true;
            else if (request_path.empty() && !argument.starts_with("--"))
                request_path = argument;
            else
                throw std::invalid_argument("Invalid command line argument: " + argument);
        }
        const auto policies = read_config(argv[1]);
        std::ifstream request_file;
        if (!request_path.empty())
            request_file = open_file(request_path);
        std::istream &input = request_path.empty() ? std::cin : request_file;
        const size_t capacity = read_number(input, "cache capacity");
        const size_t count = read_number(input, "request count");
        cache_hierarchy_t cache(make_levels(policies, capacity));
        run_requests(input, cache, count);
        print_results(cache, json);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
