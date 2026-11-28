#include "cache_hierarchy.hpp"
#include "cache_config.hpp"

#include <CLI/CLI.hpp>

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
    CLI::App app{"Compare cache policies with configurable level sizes"};
    std::string config_path, request_path, distribution = "geometric";
    std::vector<size_t> sizes;
    size_t ratio = 2;
    bool json = false;
    app.add_option("config", config_path, "Config: level count followed by policies")
        ->required()->check(CLI::ExistingFile);
    app.add_option("requests", request_path, "Input: total capacity, request count, keys; omit for stdin")
        ->check(CLI::ExistingFile);
    app.add_flag("--json", json, "Print totals and per-level statistics as JSON");
    auto *distribution_option = app.add_option("--distribution", distribution,
        "Capacity weights: equal, linear, geometric (default: geometric)")
        ->check(CLI::IsMember({"equal", "linear", "geometric"}));
    auto *ratio_option = app.add_option("--ratio", ratio, "Integer geometric growth factor (default: 2)")
        ->check(CLI::PositiveNumber);
    app.add_option("--sizes", sizes, "Exact capacities, comma-separated; sum must match input capacity")
        ->delimiter(',')->expected(1, -1)->check(CLI::NonNegativeNumber)
        ->excludes(distribution_option)->excludes(ratio_option);
    CLI11_PARSE(app, argc, argv);

    try {
        if (ratio_option->count() && distribution != "geometric")
            throw std::invalid_argument("--ratio requires geometric distribution");
        const auto policies = read_config(config_path);
        std::ifstream request_file;
        if (!request_path.empty())
            request_file = open_file(request_path);
        std::istream &input = request_path.empty() ? std::cin : request_file;
        const size_t capacity = read_number(input, "cache capacity");
        const size_t count = read_number(input, "request count");
        cache_hierarchy_t cache(make_levels(policies, capacity, distribution, ratio, sizes));
        run_requests(input, cache, count);
        print_results(cache, json);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
