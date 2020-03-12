#include <cstdio>
#include <cstdlib>
#include <new>

#include "buffer.h"

namespace snakefish {

buffer::buffer(size_t len, buffer_type type) : len(len), type(type) {
  switch (type) {
  case MALLOC:
    buf = malloc(len);
    if (buf == nullptr) {
      perror("malloc() failed");
      throw std::bad_alloc();
    }
    break;
  case MMAP:
    buf = mmap(nullptr, len, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
      perror("mmap() failed");
      throw std::bad_alloc();
    }
    break;
  default:
    fprintf(stderr, "unknown buffer type!\n");
    abort();
  }
}

buffer::~buffer() {
  switch (type) {
  case MALLOC:
    free(buf);
    break;
  case MMAP:
    if (munmap(buf, len)) {
      fprintf(stderr, "munmap() failed!");
      abort();
    }
    break;
  default:
    fprintf(stderr, "unknown buffer type!\n");
    abort();
  }
}

} // namespace snakefish
