#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <stdexcept>

#include "channel.h"
#include "util.h"

namespace snakefish {

channel::channel(const size_t size) : capacity(size) {
  // create shared memory and relevant metadata variables
  shared_mem = util::get_shared_mem(size, false);
  ref_cnt = static_cast<std::atomic_uint32_t *>(
      util::get_shared_mem(sizeof(std::atomic_uint32_t), true));
  local_ref_cnt = static_cast<std::atomic_uint32_t *>(
      util::get_mem(sizeof(std::atomic_uint32_t)));
  lock = static_cast<std::atomic_flag *>(
      util::get_shared_mem(sizeof(std::atomic_flag), true));
  start = static_cast<size_t *>(util::get_shared_mem(sizeof(size_t), true));
  end = static_cast<size_t *>(util::get_shared_mem(sizeof(size_t), true));
  full = static_cast<bool *>(util::get_shared_mem(sizeof(bool), true));
  n_unread = static_cast<sem_t *>(util::get_shared_mem(sizeof(sem_t), true));

  // initialize metadata
  new (lock) std::atomic_flag;
  *ref_cnt = 1;
  *local_ref_cnt = 1;
  *start = 0;
  *end = 0;
  *full = false;

  if (sem_init(n_unread, 1, 0)) {
    perror("sem_init() failed");
    throw std::runtime_error("sem_init() failed");
  }

  // ensure that std::atomic_uint32_t is lock free
  if (!ref_cnt->is_lock_free()) {
    fprintf(stderr, "std::atomic_uint32_t is not lock free!\n");
    abort();
  }
}

channel::~channel() {
  uint32_t global_cnt = ref_cnt->fetch_sub(1) - 1;
  uint32_t local_cnt = local_ref_cnt->fetch_sub(1) - 1;

  if (local_cnt == 0) {
    free(local_ref_cnt);
  }

  if ((local_cnt == 0) && (global_cnt == 0)) {
    if (munmap(shared_mem, capacity)) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(ref_cnt, sizeof(std::atomic_uint32_t))) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(lock, sizeof(std::atomic_flag))) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(start, sizeof(size_t))) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(end, sizeof(size_t))) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(full, sizeof(bool))) {
      perror("munmap() failed");
      abort();
    }
    if (sem_destroy(n_unread)) {
      perror("sem_destroy() failed");
      abort();
    }
    if (munmap(n_unread, sizeof(sem_t))) {
      perror("munmap() failed");
      abort();
    }
  }
}

channel::channel(const channel &t) {
  shared_mem = t.shared_mem;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;
  lock = t.lock;
  start = t.start;
  end = t.end;
  full = t.full;
  n_unread = t.n_unread;
  capacity = t.capacity;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

channel::channel(channel &&t) noexcept {
  shared_mem = t.shared_mem;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;
  lock = t.lock;
  start = t.start;
  end = t.end;
  full = t.full;
  n_unread = t.n_unread;
  capacity = t.capacity;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

void channel::send_bytes(void *bytes, size_t len) {
  // no-op
  if (len == 0)
    return;

  acquire_lock();

  // ensure that buffer is large enough
  size_t size_t_size = sizeof(size_t);
  size_t n = size_t_size + len;
  size_t head = *start;
  size_t tail = *end;
  size_t available_space = 0;
  if (head < tail)
    available_space = capacity - (tail - head);
  else if (head > tail)
    available_space = head - tail;
  else if (!(*full))
    available_space = capacity;
  if (n > available_space) {
    release_lock();
    throw std::overflow_error("channel buffer is full");
  }

  // copy the length into shared buffer
  void *len_bytes = &len;
  size_t new_end = (tail + size_t_size) % capacity;
  if (new_end > tail) {
    // no wrapping
    memcpy(static_cast<char *>(shared_mem) + tail, len_bytes, size_t_size);
  } else {
    // wrapping occurred
    size_t first_half_len = capacity - tail;
    size_t second_half_len = size_t_size - first_half_len;
    memcpy(static_cast<char *>(shared_mem) + tail, len_bytes, first_half_len);
    memcpy(shared_mem, static_cast<char *>(len_bytes) + first_half_len,
           second_half_len);
  }

  // copy the bytes into shared buffer
  tail = new_end;
  new_end = (new_end + len) % capacity;
  if (new_end > tail) {
    // no wrapping
    memcpy(static_cast<char *>(shared_mem) + tail, bytes, n);
  } else {
    // wrapping occurred
    size_t first_half_len = capacity - tail;
    size_t second_half_len = len - first_half_len;
    memcpy(static_cast<char *>(shared_mem) + tail, bytes, first_half_len);
    memcpy(shared_mem, static_cast<const char *>(bytes) + first_half_len,
           second_half_len);
  }

  // update metadata
  if (n == available_space)
    *full = true;
  *end = new_end;

  if (sem_post(n_unread)) {
    release_lock();
    perror("sem_post() failed");
    throw std::runtime_error("sem_post() failed");
  }

  release_lock();
}

void channel::send_pyobj(const py::object &obj) {
  py::object dumps = py::module::import("pickle").attr("dumps");

  // serialize obj to binary and get output
  py::object bytes = dumps(obj, PICKLE_PROTOCOL);
  PyObject *mem_view = PyMemoryView_GetContiguous(bytes.ptr(), PyBUF_READ, 'C');
  Py_buffer *buf = PyMemoryView_GET_BUFFER(mem_view);

  // send
  send_bytes(buf->buf, buf->len);
}

buffer channel::receive_bytes(const bool block) {
  if (block) {
    if (sem_wait(n_unread)) {
      perror("sem_wait() failed");
      throw std::runtime_error("sem_wait() failed");
    }
  } else {
    if (sem_trywait(n_unread)) {
      if (errno == EAGAIN) {
        throw std::out_of_range("out-of-bounds read detected");
      } else {
        perror("sem_trywait() failed");
        throw std::runtime_error("sem_trywait() failed");
      }
    }
  }
  acquire_lock();

  // get length of bytes
  size_t size_t_size = sizeof(size_t);
  buffer len_buf = buffer(size_t_size, buffer_type::MALLOC);
  void *len_bytes = len_buf.get_ptr();

  size_t head = *start;
  size_t new_start = (head + size_t_size) % capacity;
  if (new_start > head) {
    // no wrapping
    memcpy(len_bytes, static_cast<char *>(shared_mem) + head, size_t_size);
  } else {
    // wrapping occurred
    size_t first_half_len = capacity - head;
    size_t second_half_len = size_t_size - first_half_len;
    memcpy(len_bytes, static_cast<char *>(shared_mem) + head, first_half_len);
    memcpy(static_cast<char *>(len_bytes) + first_half_len, shared_mem,
           second_half_len);
  }

  // get bytes
  size_t len = *static_cast<size_t *>(len_bytes);
  buffer buf = buffer(len, buffer_type::MALLOC);
  char *bytes = static_cast<char *>(buf.get_ptr());

  head = new_start;
  new_start = (new_start + len) % capacity;
  if (new_start > head) {
    // no wrapping
    memcpy(bytes, static_cast<char *>(shared_mem) + head, len);
  } else {
    // wrapping occurred
    size_t first_half_len = capacity - head;
    size_t second_half_len = len - first_half_len;
    memcpy(bytes, static_cast<char *>(shared_mem) + head, first_half_len);
    memcpy(static_cast<char *>(bytes) + first_half_len, shared_mem,
           second_half_len);
  }

  // update metadata
  *full = false;
  *start = new_start;
  release_lock();

  return buf;
}

py::object channel::receive_pyobj(const bool block) {
  py::object loads = py::module::import("pickle").attr("loads");

  // receive & deserialize
  buffer bytes_buf = receive_bytes(block);
  py::handle mem_view = py::handle(
      PyMemoryView_FromMemory(static_cast<char *>(bytes_buf.get_ptr()),
                              bytes_buf.get_len(), PyBUF_READ));
  py::object obj = loads(mem_view);

  return obj;
}

void channel::fork() { ref_cnt->fetch_add(*local_ref_cnt); }

} // namespace snakefish
