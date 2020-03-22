#include <atomic>
#include <cstring>
#include <stdexcept>

#include "shared_buffer.h"
#include "util.h"

namespace snakefish {

shared_buffer::~shared_buffer() {
  uint32_t global_cnt = ref_cnt->fetch_sub(1) - 1;
  uint32_t local_cnt = local_ref_cnt->fetch_sub(1) - 1;

  if (local_cnt == 0) {
    free(local_ref_cnt);
  }

  if ((local_cnt == 0) && (global_cnt == 0)) {
    if (munmap(shared_mem, capacity)) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
    if (munmap(ref_cnt, sizeof(std::atomic_uint32_t))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
    if (munmap(lock, sizeof(std::atomic_flag))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
    if (munmap(start, sizeof(size_t))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
    if (munmap(end, sizeof(size_t))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
    if (munmap(full, sizeof(bool))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
  }
}

shared_buffer::shared_buffer(const shared_buffer &t) {
  shared_mem = t.shared_mem;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;
  lock = t.lock;
  start = t.start;
  end = t.end;
  full = t.full;
  capacity = t.capacity;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

shared_buffer::shared_buffer(shared_buffer &&t) noexcept {
  shared_mem = t.shared_mem;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;
  lock = t.lock;
  start = t.start;
  end = t.end;
  full = t.full;
  capacity = t.capacity;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

shared_buffer::shared_buffer(const size_t len) : capacity(len) {
  // create shared memory and relevant metadata variables
  shared_mem = util::get_shared_mem(len, false);
  ref_cnt = static_cast<std::atomic_uint32_t *>(
      util::get_shared_mem(sizeof(std::atomic_uint32_t), true));
  local_ref_cnt =
      static_cast<std::atomic_uint32_t *>(malloc(sizeof(std::atomic_uint32_t)));
  lock = static_cast<std::atomic_flag *>(
      util::get_shared_mem(sizeof(std::atomic_flag), true));
  start = static_cast<size_t *>(util::get_shared_mem(sizeof(size_t), true));
  end = static_cast<size_t *>(util::get_shared_mem(sizeof(size_t), true));
  full = static_cast<bool *>(util::get_shared_mem(sizeof(bool), true));

  // initialize metadata
  new (lock) std::atomic_flag;
  *ref_cnt = 1;
  *local_ref_cnt = 1;
  *start = 0;
  *end = 0;
  *full = false;

  // ensure that std::atomic_uint32_t is lock free
  if (!ref_cnt->is_lock_free()) {
    fprintf(stderr, "std::atomic_uint32_t is not lock free!\n");
    abort();
  }
}

void shared_buffer::read(void *buf, const size_t n) {
  // no-op
  if (n == 0)
    return;

  // spin & acquire lock
  while (lock->test_and_set())
    ;

  // prevent reading past unread bytes
  size_t head = *start;
  size_t tail = *end;
  size_t available_bytes = 0;
  if (head < tail)
    available_bytes = tail - head;
  else if (head > tail)
    available_bytes = capacity - (head - tail);
  else if (*full)
    available_bytes = capacity;
  if (n > available_bytes) {
    lock->clear();
    throw std::out_of_range("out-of-bounds read detected");
  }

  // copy the message from shared memory
  size_t new_start = (head + n) % capacity;
  if (new_start > head) {
    // no wrapping
    memcpy(buf, static_cast<char *>(shared_mem) + head, n);
  } else {
    // wrapping occurred
    size_t first_half_len = capacity - head;
    size_t second_half_len = n - first_half_len;
    memcpy(buf, static_cast<char *>(shared_mem) + head, first_half_len);
    memcpy(static_cast<char *>(buf) + first_half_len, shared_mem,
           second_half_len);
  }

  // update metadata
  *full = false;
  *start = new_start;
  lock->clear();
}

void shared_buffer::write(const void *buf, const size_t n) {
  // no-op
  if (n == 0)
    return;

  // spin & acquire lock
  while (lock->test_and_set())
    ;

  // ensure that buffer is large enough
  size_t head = *start;
  size_t tail = *end;
  size_t available_space = 0;
  if (head < tail)
    available_space = n - (tail - head);
  else if (head > tail)
    available_space = head - tail;
  else if (!(*full))
    available_space = capacity;
  if (n > available_space) {
    lock->clear();
    throw std::overflow_error("channel buffer is full");
  }

  // copy the bytes into shared buffer
  size_t new_end = (tail + n) % capacity;
  if (new_end > tail) {
    // no wrapping
    memcpy(static_cast<char *>(shared_mem) + tail, buf, n);
  } else {
    // wrapping occurred
    size_t first_half_len = capacity - tail;
    size_t second_half_len = n - first_half_len;
    memcpy(static_cast<char *>(shared_mem) + tail, buf, first_half_len);
    memcpy(shared_mem, static_cast<const char *>(buf) + first_half_len,
           second_half_len);
  }

  // update metadata
  if (n == available_space)
    *full = true;
  *end = new_end;
  lock->clear();
}

void shared_buffer::fork() { ref_cnt->fetch_add(*local_ref_cnt); }

} // namespace snakefish
