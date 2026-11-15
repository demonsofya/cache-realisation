#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

struct cache_level_config_t {
  std::string policy;
  size_t capacity;
};

struct cache_level_stats_t {
  cache_level_config_t config;
  size_t hits = 0;
  size_t misses = 0;
};

// Read-only simulator: values equal keys; levels replace entries independently.
class cache_hierarchy_t {
public:
  explicit cache_hierarchy_t(const std::vector<cache_level_config_t> &config);
  ~cache_hierarchy_t();
  cache_hierarchy_t(const cache_hierarchy_t &) = delete;
  cache_hierarchy_t &operator=(const cache_hierarchy_t &) = delete;

  bool lookup_update(size_t key);
  const std::vector<cache_level_stats_t> &levels() const;
  size_t requests() const;
  size_t memory_accesses() const;

private:
  struct level_t;
  template <typename Cache> struct level_impl_t;

  std::vector<std::unique_ptr<level_t>> caches_;
  std::vector<cache_level_stats_t> stats_;
  size_t requests_ = 0;
  size_t memory_accesses_ = 0;
};
