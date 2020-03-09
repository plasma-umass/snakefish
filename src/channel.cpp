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
  int socket_fd[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, socket_fd)) {
    perror("socketpair() failed");
    throw std::runtime_error("socketpair() failed");
  }

  if (fcntl(socket_fd[0], F_SETFD, FD_CLOEXEC)) {
    perror("fcntl(FD_CLOEXEC) failed for sender socket");
    throw std::runtime_error("fcntl(FD_CLOEXEC) failed for sender socket");
  }
  if (fcntl(socket_fd[1], F_SETFD, FD_CLOEXEC)) {
    perror("fcntl(FD_CLOEXEC) failed for receiver socket");
    throw std::runtime_error("fcntl(FD_CLOEXEC) failed for receiver socket");
  }

  return {sender(socket_fd[0]), receiver(socket_fd[0]),
          sender(socket_fd[1]), receiver(socket_fd[1])};
}

void sender::send_bytes(const void *bytes, const size_t len) {
  ssize_t result = send(socket_fd, bytes, len, 0);
  if (result == -1) {
    perror("send() failed");
    throw std::runtime_error("send() failed");
  } else if ((size_t)result != len) {
    fprintf(stderr, "sent %ld bytes, should be %ld bytes!\n", result, len);
    abort();
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
  if (close(socket_fd)) {
    perror("close() failed");
    throw std::runtime_error("close() failed");
  }
}

void *receiver::receive_bytes(const size_t len) {
  void *bytes = malloc(len);
  if (bytes == nullptr) {
    perror("malloc() failed");
    throw std::bad_alloc();
  }

  ssize_t result = recv(socket_fd, bytes, len, 0);
  if (result == -1) {
    perror("recv() failed");
    throw std::runtime_error("recv() failed");
  } else if ((size_t)result != len) {
    fprintf(stderr, "received %ld bytes, should be %ld bytes!\n", result, len);
    abort();
  }

  return bytes;
}

py::object receiver::receive_pyobj() {
  py::object loads = py::module::import("pickle").attr("loads");

  // receive input size
  Py_ssize_t *size_ptr =
      reinterpret_cast<Py_ssize_t *>(receive_bytes(sizeof(Py_ssize_t)));
  Py_ssize_t size = *size_ptr;
  free(size_ptr);

  // receive input
  char *bytes = reinterpret_cast<char *>(receive_bytes(size));
  py::handle mem_view =
      py::handle(PyMemoryView_FromMemory(bytes, size, PyBUF_READ));
  py::object obj = loads(mem_view);
  free(bytes);

  return obj;
}

void receiver::dispose() {
  if (close(socket_fd)) {
    perror("close() failed");
    throw std::runtime_error("close() failed");
  }
}

} // namespace snakefish
