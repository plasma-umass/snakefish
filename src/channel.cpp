#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <stdexcept>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "channel.h"

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

  return {sender(socket_fd[0], shared_mem), receiver(socket_fd[0], shared_mem),
          sender(socket_fd[1], shared_mem), receiver(socket_fd[1], shared_mem)};
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

void sender::dispose() {
  // Currently, a sender holds the same socket fd as its
  // corresponding receiver. As such, it makes sense
  // for the receiver to close() the socket fd, and the
  // sender's dispose() should just be a no-op.
  //
  (void)(this);
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

void receiver::dispose() {
  if (close(socket_fd)) {
    perror("close() failed");
    throw std::runtime_error("close() failed");
  }
}

} // namespace snakefish
