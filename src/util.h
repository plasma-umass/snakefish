#ifndef SNAKEFISH_UTIL_H
#define SNAKEFISH_UTIL_H

#include <cstdio>
#include <new>
#include <random>

#include <sys/mman.h>

namespace snakefish {

namespace util {

static uint64_t get_random_uint() {
  static std::random_device rd;
  static std::mt19937 rng(rd());
  static std::uniform_int_distribution<std::mt19937::result_type> dist;
  return dist(rng);
}

static void *get_mem(const size_t len) {
  // no-op
  if (len == 0)
    return nullptr;

  void *p = malloc(len);
  if (p == nullptr) {
    perror("malloc() failed");
    throw std::bad_alloc();
  } else {
    return p;
  }
}

static void *get_shared_mem(const size_t len, const bool reserve) {
  // no-op
  if (len == 0)
    return nullptr;

  void *mem;
  if (reserve) {
    mem = mmap(nullptr, len, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  } else {
    mem = mmap(nullptr, len, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  }

  if (mem == MAP_FAILED) {
    perror("mmap() failed");
    throw std::bad_alloc();
  } else {
    return mem;
  }
}

} // namespace util

} // namespace snakefish

#endif // SNAKEFISH_UTIL_H
