#ifndef SNAKEFISH_UTIL_H
#define SNAKEFISH_UTIL_H

#include <cstdio>
#include <new>

#include <sys/mman.h>

namespace snakefish {

namespace util {

static void *get_shared_mem(const size_t len, const bool reserve) {
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
