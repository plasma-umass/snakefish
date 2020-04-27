/**
 * \file util.h
 */

#ifndef SNAKEFISH_UTIL_H
#define SNAKEFISH_UTIL_H

#include <cstdio>
#include <new>
#include <random>

#include <sys/mman.h>

namespace snakefish {

namespace util {

/**
 * \brief Randomly generate an `uint`.
 */
static inline uint64_t get_random_uint() {
  static std::random_device rd;
  static std::mt19937 rng(rd());
  static std::uniform_int_distribution<std::mt19937::result_type> dist;
  return dist(rng);
}

/**
 * \brief Use `malloc()` to allocate some memory.
 *
 * \param len Number of bytes to allocate.
 *
 * \returns Pointer to the start of the allocated memory, which is `null` if
 * `len` is 0.
 *
 * \throws std::bad_alloc If `malloc()` failed.
 */
static inline void *get_mem(const size_t len) {
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

/**
 * \brief Use `mmap()` to allocate some shared memory.
 *
 * \param len Number of bytes to allocate.
 * \param reserve If `false`, use flag `MAP_NORESERVE`.
 *
 * \returns Pointer to the start of the allocated memory, which is `null` if
 * `len` is 0.
 *
 * \throws std::bad_alloc If `mmap()` failed.
 */
static inline void *get_shared_mem(const size_t len, const bool reserve) {
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
