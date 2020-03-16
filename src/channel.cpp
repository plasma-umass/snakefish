#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <stdexcept>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "channel.h"
#include "util.h"

namespace snakefish {

std::tuple<sender, receiver, sender, receiver> sync_channel() {
  return sync_channel(DEFAULT_CHANNEL_SIZE);
}

std::tuple<sender, receiver, sender, receiver>
sync_channel(const size_t channel_size) {
  // open unix domain sockets
  int socket_fd[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, socket_fd)) {
    perror("socketpair() failed");
    throw std::runtime_error("socketpair() failed");
  }

  // update settings so sockets would close on exec()
  if (fcntl(socket_fd[0], F_SETFD, FD_CLOEXEC)) {
    perror("fcntl(FD_CLOEXEC) failed for sender socket");
    throw std::runtime_error("fcntl(FD_CLOEXEC) failed for sender socket");
  }
  if (fcntl(socket_fd[1], F_SETFD, FD_CLOEXEC)) {
    perror("fcntl(FD_CLOEXEC) failed for receiver socket");
    throw std::runtime_error("fcntl(FD_CLOEXEC) failed for receiver socket");
  }

  // create shared buffer
  shared_buffer shared_mem(channel_size);

  // create and initialize metadata variables
  auto ref_cnt = static_cast<std::atomic_uint32_t *>(
      get_shared_mem(sizeof(std::atomic_uint32_t), true));
  auto local_ref_cnt =
      static_cast<std::atomic_uint32_t *>(malloc(sizeof(std::atomic_uint32_t)));
  auto local_ref_cnt2 =
      static_cast<std::atomic_uint32_t *>(malloc(sizeof(std::atomic_uint32_t)));
  *ref_cnt = 4;
  *local_ref_cnt = 2;
  *local_ref_cnt2 = 2;

  return {sender(socket_fd[0], shared_mem, ref_cnt, local_ref_cnt),
          receiver(socket_fd[0], shared_mem, ref_cnt, local_ref_cnt),
          sender(socket_fd[1], shared_mem, ref_cnt, local_ref_cnt2),
          receiver(socket_fd[1], shared_mem, ref_cnt, local_ref_cnt2)};
}

// Currently, a sender holds the same socket fd as its
// corresponding receiver. As such, it makes sense
// for the receiver to close() the socket fd
sender::~sender() {
  uint32_t global_cnt = ref_cnt->fetch_sub(1) - 1;
  uint32_t local_cnt = local_ref_cnt->fetch_sub(1) - 1;

  if (local_cnt == 0) {
    free(local_ref_cnt);
  }

  if ((local_cnt == 0) && (global_cnt == 0)) {
    if (munmap(ref_cnt, sizeof(std::atomic_uint32_t))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
  }
}

sender::sender(const sender &t) : shared_mem(t.shared_mem) {
  socket_fd = t.socket_fd;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

sender::sender(sender &&t) noexcept : shared_mem(std::move(t.shared_mem)) {
  socket_fd = t.socket_fd;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

void sender::send_bytes(const void *bytes, const size_t len) {
  if (len <= MAX_SOCK_MSG_SIZE) {
    // for small messages, just send them through sockets
    ssize_t result = send(socket_fd, bytes, len, 0);
    if (result == -1) {
      perror("send() failed");
      throw std::runtime_error("send() failed");
    } else if ((size_t)result != len) {
      fprintf(stderr, "sent %ld bytes, should be %ld bytes!\n", result, len);
      abort();
    }
  } else {
    // for large messages, send them through shared memory
    shared_mem.write(bytes, len);
  }
}

void sender::send_pyobj(const py::object &obj) {
  py::object dumps = py::module::import("pickle").attr("dumps");

  // serialize obj to binary and get output
  py::object bytes = dumps(obj, PICKLE_PROTOCOL);
  PyObject *mem_view = PyMemoryView_GetContiguous(bytes.ptr(), PyBUF_READ, 'C');
  Py_buffer *buf = PyMemoryView_GET_BUFFER(mem_view);

  // send
  send_bytes(&(buf->len), sizeof(Py_ssize_t)); // output size
  send_bytes(buf->buf, buf->len);              // output
}

void sender::fork() {
  ref_cnt->fetch_add(*local_ref_cnt);
  shared_mem.fork();
}

receiver::~receiver() {
  uint32_t global_cnt = ref_cnt->fetch_sub(1) - 1;
  uint32_t local_cnt = local_ref_cnt->fetch_sub(1) - 1;

  if (local_cnt == 0) {
    if (close(socket_fd)) {
      perror("close() failed");
      abort();
    }
    free(local_ref_cnt);
  }

  if ((local_cnt == 0) && (global_cnt == 0)) {
    if (munmap(ref_cnt, sizeof(std::atomic_uint32_t))) {
      fprintf(stderr, "munmap() failed!\n");
      abort();
    }
  }
}

receiver::receiver(const receiver &t) : shared_mem(t.shared_mem) {
  socket_fd = t.socket_fd;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

receiver::receiver(receiver &&t) noexcept
    : shared_mem(std::move(t.shared_mem)) {
  socket_fd = t.socket_fd;
  ref_cnt = t.ref_cnt;
  local_ref_cnt = t.local_ref_cnt;

  // increment reference counters
  ref_cnt->fetch_add(1);
  local_ref_cnt->fetch_add(1);
}

buffer receiver::receive_bytes(const size_t len) {
  buffer bytes = buffer(len, buffer_type::MALLOC);
  char *buf = static_cast<char *>(bytes.get_ptr());

  if (len <= MAX_SOCK_MSG_SIZE) {
    // for small messages, just receive them through sockets
    ssize_t result = recv(socket_fd, buf, len, 0);
    if (result == -1) {
      perror("recv() failed");
      throw std::runtime_error("recv() failed");
    } else if ((size_t)result != len) {
      fprintf(stderr, "received %ld bytes, should be %ld bytes!\n", result,
              len);
      abort();
    }
  } else {
    // for large messages, receive them from shared memory
    shared_mem.read(buf, len);
  }

  return bytes;
}

py::object receiver::receive_pyobj() {
  py::object loads = py::module::import("pickle").attr("loads");

  // receive input size
  buffer size_buf = receive_bytes(sizeof(Py_ssize_t));
  Py_ssize_t size = *static_cast<Py_ssize_t *>(size_buf.get_ptr());

  // receive input
  buffer bytes_buf = receive_bytes(size);
  py::handle mem_view = py::handle(PyMemoryView_FromMemory(
      static_cast<char *>(bytes_buf.get_ptr()), size, PyBUF_READ));
  py::object obj = loads(mem_view);

  return obj;
}

void receiver::fork() {
  ref_cnt->fetch_add(*local_ref_cnt);
  shared_mem.fork();
}

} // namespace snakefish
