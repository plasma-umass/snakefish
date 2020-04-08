#include "thread.h"
#include "snakefish.h"
#include "util.h"

namespace snakefish {

std::set<thread *> all_threads;

thread::~thread() {
  all_threads.erase(this);

  if (is_parent) {
    if (munmap(alive, sizeof(std::atomic_bool))) {
      perror("munmap() failed");
      abort();
    }
  }
}

thread::thread(py::function f, py::function extract, py::function merge)
    : is_parent(false), child_pid(0), started(false), joined(false),
      child_status(0), func(std::move(f)), extract_func(std::move(extract)),
      merge_func(std::move(merge)), _channel() {
  // create shared memory
  alive = static_cast<std::atomic_bool *>(
      util::get_shared_mem(sizeof(std::atomic_bool), true));
  alive->store(false);

  all_threads.insert(this);
}

void thread::start() {
  if (started) {
    throw std::runtime_error("this thread has already been started");
  }

  pid_t pid = snakefish_fork();
  if (pid > 0) {
    is_parent = true;
    child_pid = pid;
    started = true;
  } else if (pid == 0) {
    is_parent = false;
    child_pid = 0;
    started = true;
    run();
  } else {
    perror("fork() failed");
    throw std::runtime_error("fork() failed");
  }
}

void thread::join() {
  if (!started) {
    throw std::runtime_error("this thread has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "join() called from child!\n");
    abort();
  }

  int result = waitpid(child_pid, &child_status, 0);
  if (result == -1) {
    perror("waitpid() failed");
    throw std::runtime_error("waitpid() failed");
  } else if (result != child_pid) {
    fprintf(stderr, "joined pid = %d, child pid = %d!\n", result, child_pid);
    abort();
  } else {
    joined = true;
    globals = _channel.receive_pyobj(true);
    ret_val = _channel.receive_pyobj(true);
    merge_func(py::globals(), globals);
  }
}

bool thread::try_join() {
  if (!started) {
    throw std::runtime_error("this thread has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "try_join() called from child!\n");
    abort();
  }

  int result = waitpid(child_pid, &child_status, WNOHANG);
  if (result == 0) {
    return false;
  } else if (result == -1) {
    perror("waitpid() failed");
    throw std::runtime_error("waitpid() failed");
  } else if (result != child_pid) {
    fprintf(stderr, "joined pid = %d, child pid = %d!\n", result, child_pid);
    abort();
  } else {
    joined = true;
    globals = _channel.receive_pyobj(true);
    ret_val = _channel.receive_pyobj(true);
    merge_func(py::globals(), globals);
    return true;
  }
}

int thread::get_exit_status() {
  if (!started) {
    return -1;
  } else if (!joined) {
    return -2;
  } else if (WIFEXITED(child_status)) {
    return WEXITSTATUS(child_status);
  } else {
    return -3;
  }
}

void thread::run() {
  if (is_parent) {
    fprintf(stderr, "run() called by parent!\n");
    abort();
  }
  if (!started) {
    fprintf(stderr, "run() called but thread hasn't started yet!\n");
    abort();
  }

  alive->store(true);
  try {
    ret_val = func();
    globals = extract_func(py::globals());

    _channel.send_pyobj(globals);
    _channel.send_pyobj(ret_val);
  } catch (py::error_already_set &e) {
    // send globals
    globals = extract_func(py::globals());
    _channel.send_pyobj(globals);

    // send exceptions to parent
    _channel.send_pyobj(e.value());
    _channel.send_pyobj(e.type());

    // send traceback
    if (e.trace()) {
      _channel.send_pyobj(
          py::module::import("traceback")
              .attr("format_exception")(e.type(), e.value(), e.trace()));
    } else {
      _channel.send_pyobj(
          py::module::import("traceback")
              .attr("format_exception_only")(e.type(), e.value()));
    }
  }
  alive->store(false);

  snakefish_exit(0);
}

py::object thread::get_result() {
  if ((!started) || (!joined)) {
    throw std::runtime_error("result is not yet available");
  }

  if (py::isinstance(ret_val, PyExc_Exception)) {
    // handle exceptions
    py::object type = _channel.receive_pyobj(true);
    py::object traceback = _channel.receive_pyobj(true);

    py::print(py::str("").attr("join")(traceback));
    PyErr_SetObject(type.ptr(), ret_val.ptr());
    throw py::error_already_set();
  } else {
    return ret_val;
  }
}

} // namespace snakefish
