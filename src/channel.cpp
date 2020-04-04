#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <stdexcept>

#include "channel.h"
#include "util.h"

namespace snakefish {

std::set<channel *> all_channels;

channel::channel(const size_t size) : n_unread(), capacity(size) {
  // imports pickle functions
  dumps = py::module::import("pickle").attr("dumps");
  loads = py::module::import("pickle").attr("loads");

  // create shared memory and relevant metadata variables
  shared_mem = util::get_shared_mem(size, false);
  ref_cnt = static_cast<std::atomic_uint32_t *>(
      util::get_shared_mem(sizeof(std::atomic_uint32_t), true));
  local_ref_cnt = static_cast<std::atomic_uint32_t *>(
      util::get_mem(sizeof(std::atomic_uint32_t)));
  lock = static_cast<std::atomic_flag *>(
      util::get_shared_mem(sizeof(std::atomic_flag), true));
  start = static_cast<std::atomic_size_t *>(
      util::get_shared_mem(sizeof(std::atomic_size_t), true));
  end = static_cast<std::atomic_size_t *>(
      util::get_shared_mem(sizeof(std::atomic_size_t), true));
  full = static_cast<std::atomic_bool *>(
      util::get_shared_mem(sizeof(std::atomic_bool), true));

  // initialize metadata
  new (lock) std::atomic_flag;
  ref_cnt->store(1);
  local_ref_cnt->store(1);
  start->store(0);
  end->store(0);
  full->store(false);

  // ensure that shared atomic variables are lock free
  // note that atomic_flag is always lock free
  // also note that end is of the same type as start
  if (!ref_cnt->is_lock_free()) {
    fprintf(stderr, "std::atomic_uint32_t is not lock free!\n");
    abort();
  }
  if (!start->is_lock_free()) {
    fprintf(stderr, "std::atomic_size_t is not lock free!\n");
    abort();
  }
  if (!full->is_lock_free()) {
    fprintf(stderr, "std::atomic_bool is not lock free!\n");
    abort();
  }

  all_channels.insert(this);
}

channel::~channel() {
  all_channels.erase(this);

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
    if (munmap(start, sizeof(std::atomic_size_t))) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(end, sizeof(std::atomic_size_t))) {
      perror("munmap() failed");
      abort();
    }
    if (munmap(full, sizeof(std::atomic_bool))) {
      perror("munmap() failed");
      abort();
    }
    try {
      n_unread.destroy();
    } catch (...) {
      abort();
    }
  }
}

channel::channel(const channel &t) : n_unread(t.n_unread) {
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

  all_channels.insert(this);
}

channel::channel(channel &&t) noexcept : n_unread(std::move(t.n_unread)) {
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

  all_channels.insert(this);
}

void channel::send_bytes(void *bytes, size_t len) {
  // no-op
  if (len == 0)
    return;

  acquire_lock();

  // ensure that buffer is large enough
  size_t size_t_size = sizeof(size_t);
  size_t n = size_t_size + len;
  size_t head = start->load();
  size_t tail = end->load();
  size_t available_space = 0;
  if (head < tail)
    available_space = capacity - (tail - head);
  else if (head > tail)
    available_space = head - tail;
  else if (!(full->load()))
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
    full->store(true);
  end->store(new_end);

  try {
    n_unread.post();
  } catch (const std::runtime_error &e) {
    release_lock();
    throw e;
  }

  release_lock();
}

void channel::send_pyobj(const py::object &obj) {
  // serialize obj to binary and get output
  py::object bytes = dumps(obj, PICKLE_PROTOCOL);
  PyObject *mem_view = PyMemoryView_GetContiguous(bytes.ptr(), PyBUF_READ, 'C');
  Py_buffer *buf = PyMemoryView_GET_BUFFER(mem_view);

  // send
  send_bytes(buf->buf, buf->len);
}

buffer channel::receive_bytes(const bool block) {
  if (block) {
    n_unread.wait();
  } else {
    if (!n_unread.trywait()) {
      throw std::out_of_range("out-of-bounds read detected");
    }
  }
  acquire_lock();

  // get length of bytes
  size_t size_t_size = sizeof(size_t);
  buffer len_buf = buffer(size_t_size, buffer_type::MALLOC);
  void *len_bytes = len_buf.get_ptr();

  size_t head = start->load();
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
    memcpy(bytes + first_half_len, shared_mem, second_half_len);
  }

  // update metadata
  full->store(false);
  start->store(new_start);
  release_lock();

  return buf;
}

py::object channel::receive_pyobj(const bool block) {
  // receive & deserialize
  buffer bytes_buf = receive_bytes(block);
  py::handle mem_view = py::handle(
      PyMemoryView_FromMemory(static_cast<char *>(bytes_buf.get_ptr()),
                              bytes_buf.get_len(), PyBUF_READ));
  py::object obj = loads(mem_view);

  return obj;
}

void channel::fork() { ref_cnt->fetch_add(local_ref_cnt->load()); }

} // namespace snakefish
